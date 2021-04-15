#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"

bool decode_fwip( fwip_s * fwip, uint8_t * in );
bool send_dr( bool verified, uint8_t * buffer );
void client( internet_s * internet );

#endif