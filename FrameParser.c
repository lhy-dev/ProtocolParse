#include <stdlib.h>
#include <stdbool.h>

#include "FrameParser.h"

#define START_CHAR							0x02
#define END_CHAR							0x03
#define PADDING_CHAR						0xAA

#define min(x,y)  (( x < y )?x:y)

enum rx_state
{
	RX_STATE_START = 0,
	RX_STATE_PADDING = 1,
	RX_STATE_MESSAGE_ID = 2,
	RX_STATE_SEQUENCE_ID = 3,
	RX_STATE_DATA_SIZE = 4,
	RX_STATE_RESERVED = 5,
	RX_STATE_HEAD_CHECKSUM = 6,
	RX_STATE_END = 7,
	RX_STATE_DATA = 8,
};
typedef enum rx_state RX_STATE;

uint8_t CheckSum ( char* buf, int length )
{
	uint8_t checkSum = 0;

	if ( buf == NULL )
	{
		return 0;
	}
	for ( int i = 0; i < length; i++ )
	{
		checkSum += (uint8_t) buf[ i ];
	}

	return checkSum;
}

int FrameUnpack ( char* dataBuffer, int dataLength )
{
	static RX_STATE rx_state = RX_STATE_START;
	static uint8_t rx_checksum;
	static int rx_index;
	static uint16_t rx_msgid;
	static uint16_t rx_seqId;
	static uint16_t rx_datasize;
	static uint8_t rx_Data[ RESERVE_BUFFER_SIZE ];

	int rcv_count = 1;
	bool complete = false;

	switch ( rx_state )
	{
	case RX_STATE_START:
		if ( START_CHAR == dataBuffer[ 0 ] )
		{
			rx_state = RX_STATE_PADDING;
			rx_checksum = (uint8_t) dataBuffer[ 0 ];
		}
		break;

	case RX_STATE_PADDING:
		if ( PADDING_CHAR == dataBuffer[ 0 ] )
		{
			rx_checksum += (uint8_t) dataBuffer[ 0 ];
			rx_index = 0;
			rx_state = RX_STATE_MESSAGE_ID;
		}
		else
		{
			rx_state = RX_STATE_START;
		}
		break;

	case RX_STATE_MESSAGE_ID:
		rx_checksum += (uint8_t) dataBuffer[ 0 ];

		if ( 0 == rx_index )
		{
			rx_msgid = (uint8_t) dataBuffer[ 0 ];
			rx_index++;
		}
		else
		{
			uint16_t tmp = (uint8_t) dataBuffer[ 0 ];
			tmp <<= 8;
			rx_msgid += tmp;

			rx_index = 0;
			rx_state = RX_STATE_SEQUENCE_ID;
		}
		break;

	case RX_STATE_SEQUENCE_ID:
		rx_checksum += (uint8_t) dataBuffer[ 0 ];

		if ( 0 == rx_index )
		{
			rx_seqId = (uint8_t) dataBuffer[ 0 ];
			rx_index++;
		}
		else
		{
			uint16_t tmp = (uint8_t) dataBuffer[ 0 ];
			tmp <<= 8;
			rx_seqId += tmp;

			rx_index = 0;
			rx_state = RX_STATE_DATA_SIZE;
		}
		break;

	case RX_STATE_DATA_SIZE:
		rx_checksum += (uint8_t) dataBuffer[ 0 ];

		if ( 0 == rx_index )
		{
			rx_datasize = (uint8_t) dataBuffer[ 0 ];
			rx_index++;
		}
		else
		{
			uint16_t tmp = (uint8_t) dataBuffer[ 0 ];
			tmp <<= 8;
			rx_datasize += tmp;

			if ( rx_datasize > RESERVE_BUFFER_SIZE )
			{
				rx_state = RX_STATE_START;
			}
			else
			{
				rx_state = RX_STATE_RESERVED;
			}
		}
		break;

	case RX_STATE_RESERVED:
		rx_checksum += (uint8_t) dataBuffer[ 0 ];

		if ( 0 == rx_index )
		{
			rx_index++;
		}
		else
		{
			rx_index = 0;
			rx_state = RX_STATE_HEAD_CHECKSUM;
		}
		break;

	case RX_STATE_HEAD_CHECKSUM:
		if ( rx_checksum != dataBuffer[ 0 ] )
		{
			rx_state = RX_STATE_START;
		}
		else
		{
			rx_state = RX_STATE_END;
		}
		break;

	case RX_STATE_END:
		if ( END_CHAR == dataBuffer[ 0 ] )
		{
			if ( rx_datasize > 0 )
			{
				rx_index = 0;
				rx_checksum = 0;
				rx_state = RX_STATE_DATA;
			}
			else
			{
				complete = true;
				rx_state = RX_STATE_START;
			}
		}
		else
		{
			rx_state = RX_STATE_START;
		}
		break;

	case RX_STATE_DATA:
		rcv_count = min( rx_datasize - rx_index, dataLength );
		memcpy( &rx_Data[ rx_index ], dataBuffer, rcv_count );
		rx_index += rcv_count;

		if ( rx_index >= rx_datasize )
		{
			complete = true;
			rx_state = RX_STATE_START;
		}
		break;

	default:
		rx_state = RX_STATE_START;
		break;
	}

	if ( complete )
	{
		do
		{
			if ( rx_datasize > 1 )
			{
				rx_checksum = 0;
				for ( int i = 0; i < rx_datasize - 1; i++ )
				{
					rx_checksum += rx_Data[ i ];
				}

				if ( rx_checksum != rx_Data[ rx_datasize - 1 ] )
				{
					break;
				}
			}

			if ( rx_datasize < 2 )
			{
				rx_datasize = 0;
			}
			else
			{
				rx_datasize -= 1;
			}
		} while ( 0 );
	}

	return rcv_count;
}

char* FramePack (
	uint8_t messageID,
	uint16_t sequenceID,
	char* data,
	uint16_t dataLength,
	char** pkgData,
	uint32_t pkgDataLength )
{
	int sd_size = 12 + dataLength + 1;
	char* package = malloc( sd_size );
	package[ 0 ] = START_CHAR;
	package[ 1 ] = PADDING_CHAR;
	package[ 2 ] = (char) messageID;
	package[ 3 ] = messageID >> 8;
	package[ 4 ] = (char) sequenceID;
	package[ 5 ] = sequenceID >> 8;
	package[ 6 ] = (char) ( dataLength + 1 );
	package[ 7 ] = ( dataLength + 1 ) >> 8;
	package[ 8 ] = 0x00;
	package[ 9 ] = 0x00;
	package[ 10 ] = CheckSum( package, 10 );
	package[ 11 ] = PADDING_CHAR;
	if ( dataLength > 0 )
	{
		memcpy( package + 12, data, dataLength );
		package[ dataLength + 12 ] = CheckSum( package + 12, dataLength );
		pkgDataLength = dataLength + 12 + 1;
	}

	*pkgData = package;

	return ( *pkgData );
}

