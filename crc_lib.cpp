#include <stdint.h>
#include "crc_lib.h"

/*---------------------------------------------------------------------------*/
/* Implementation from https://github.com/u-boot/u-boot/blob/master/lib/crc16.c
 * Calculate crc of len bytes of a uin8_t buffer, given an initial cheksum.
 * Return the result. */
uint16_t crc16( uint16_t cksum, const uint8_t * buf, unsigned int len )
{
	for( unsigned int i = 0;  i < len;  i++ ) {
		cksum = crc16_tab[ ( ( cksum >> 8 ) ^ *buf++ ) & 0xff ] ^ ( cksum << 8 );
    }
	return cksum;
}

/* Convert a uin16_t into 2 uint8_t bytes. Note that we're keeping it little
 * endian. */
void uint16_to_arr( uint16_t in, uint8_t * out )
{
    out[ 0 ] = in & 0xFF;
    out[ 1 ] = ( in >> 8 ) & 0xFF;
    return;
}

/* Convert a uint8_t[2] array into a uint16.  Return the result. */
uint16_t arr_to_uint16( uint8_t * in )
{
    uint16_t out = in[ 0 ] | in[ 1 ] << 8;
    return out;
}
