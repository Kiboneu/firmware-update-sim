#include "nanopb_wrapper.cpp"
#include "crc_lib.h"

#include "client.h"

/*---------------------------------------------------------------------------*/
/* Client encode / decode helper functions                                   */

/* Encode the provided boolian through nanopb, and store that result into a 
 * uint8_t buffer. Return false on error. */
bool send_dr( bool verified, uint8_t * buffer )
{
    memset( buffer, 0, gridware_FirmwareImagePage_size );
    pb_ostream_t ostream = pb_ostream_from_buffer( buffer, gridware_DeviceResponse_size );
    if( ! encode( verified, gridware_DeviceResponse_verified_tag, PB_WT_VARINT, &ostream ) )
    {
        return false;
    }
    return true;
}

/* Read a uint8_t array with protobuf encoded contents into a fwip_s struct
 * Return false on decode tag or decode error. */
bool decode_fwip(fwip_s * fwip, uint8_t * in)
{
    pb_istream_t istream = pb_istream_from_buffer(in, gridware_FirmwareImagePage_size );
    pb_wire_type_t wire;

    /* If pb_decode_tag fails tag will remain zero and helpfully indicate an
     * issue, which can be handled in the default switch case below. */
    uint32_t tag = 0;
    bool eof = 0;

    while (pb_decode_tag(&istream, &wire, &tag, &eof) && istream.bytes_left > 0) {
        switch(tag) {
            case gridware_FirmwareImagePage_page_tag:
                if( ! decode( fwip->page, PB_WT_STRING, &istream, sizeof(fwip->page) ) )
                {
                    return false;
                }
                break;

            case gridware_FirmwareImagePage_crc_tag:
                if( ! decode( fwip->crc, PB_WT_STRING, &istream, sizeof(fwip->crc) ) )
                {
                    return false;
                }
                break;

            case gridware_FirmwareImagePage_last_tag:
                if( ! decode( &fwip->last, PB_WT_VARINT, &istream ) )
                {
                    return false;
                }
                break;

            default:
                printf( "decode fwip Unknown tag encountered %#x\n", tag );
                return false;
        }
	}
    return true; // TODO: Maybe return EOF instead
}

/*---------------------------------------------------------------------------*/
/* Client firmware page processing loop                                      */

/* Process a uint8 buffer containing protbuf into a fwip_s, calculate the crc
 * of the received page, compare the crc with the fwip->crc and send the
 * comparison result as a protobuf encoded boolean to the client.
 * The client will also set verified = 0 if there's a decoding error.
 */
void client( internet_s * internet )
{
    bool is_listening = true;
    while( is_listening )
    {
        /* Wait for the server to release mutex before proceeding.
         * In real life this would be a HW interript. */
        while( internet->owner != CLIENT ) { };

        fwip_s fwip = { 0 };
        bool verified = 0;
        
        /* Attempt to decode the fwip. */
        if( ! decode_fwip( &fwip, internet->buffer ) )
        {
            printf( "\nCLIENT : decode_fwip failed.\n" );
            // printf("CLIENT : fwip->buffer =\n%s\n", fwip->page);
            printf("CLIENT : fwip->crc    = %#x\n", arr_to_uint16(fwip.crc));
            printf("CLIENT : fwip->last   = %#x\n", fwip.last);
            printf("\n");
        }
        else /* Calculate and compare the crc of the received page and fwip->crc */
        {
            uint16_t client_crc = crc16( 0, fwip.page, SZ_FW_PAGE ); // TODO: Move instantiations outside loop
            uint16_t server_crc = arr_to_uint16( fwip.crc );

            verified = ( client_crc == server_crc ) ? true : false;
            if( ! verified )
            {
                g_fail_count++;
                printf( "CLIENT : verification failed.\n");
            }

            printf( "CLIENT : decoded fields:\n" );
            printf( "CLIENT : fwip->crc    = %#x\n", server_crc );
            printf( "CLIENT : fwip->last   = %#x\n", fwip.last );
            printf( "CLIENT : server_crc (%#x) == client_crc (%#x) ? %#x\n\n", server_crc, client_crc, verified );
        }

        if( fwip.last )
        {
            printf( "CLIENT : Processed last page. We're finished here.\n" );
            is_listening = false;
        }

        send_dr( verified, internet->buffer ); /* A device response is always expected */

        /* Nothing else to do, so return ownership of the internet to client. */
        internet->owner = SERVER;
    }
    printf("CLIENT : Terminating connection.\n");
    return;
}
