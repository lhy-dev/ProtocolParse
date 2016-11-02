#ifndef __FRAME_PARSER_H_
#define __FRAME_PARSER_H_

#ifdef  __cpluscplus
extern "C"
{
#endif

#include <stdint.h>
#include <string.h>

/* Package format
 +------------------------+
 0	|  start_char <0x02>  |
 +------------------------+
 1	| padding_char <0xAA> |
 +------------------------+
 2	|   message_id_lsb    |
 +------------------------+
 3	|   message_id_msb    |
 +------------------------+
 4	|   sequence_id_lsb   |
 +------------------------+
 5	|   sequence_id_msb   |
 +------------------------+
 6	|       size_lsb      |
 +------------------------+
 7	|       size_lsb      |
 +------------------------+
 8	|       reserved      |
 +------------------------+
 9	|       reserved      |
 +------------------------+
 10	|    head_check_sum   |
 +------------------------+
 11	|    end_char <0x03>  |
 +------------------------+
 12	|        data         |
 +------------------------+
 |          ...           |
 +------------------------+
 N	|    data_check_sum   |
 +------------------------+
 */

#define RECV_BUFFER_SIZE		(1024*64)
#define RESERVE_BUFFER_SIZE		(RECV_BUFFER_SIZE*2)
#define FRAME_HEAD_LENGTH		12

extern int FrameUnpack( char* dataBuffer, int datalength );

extern char* FramePack(
	uint8_t messageID,
	uint16_t sequenceID,
	char* data,
	uint16_t datalength,
	char** pkgdata,
	uint32_t pkgDatalength );

#ifdef  __cpluscplus
}
#endif

#endif  // __FRAME_PARSER_H_
