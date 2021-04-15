#ifndef SERVER_H
#define SERVER_H

#include "common.h"

bool encode_fwip( fwip_s * fwip, uint8_t * out);
bool send_fwip( fwip_s * fwip, uint8_t * buffer );
bool decode_dr( bool * verified, uint8_t * in );
bool server( internet_s * internet, uint8_t * page );
#endif
