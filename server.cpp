#include <cstdio> // Using funcs likely existing on a nostd lib (i.e. newlib)
#include <thread>

// TODO: Implement as a class instead. Internally templated funcs get converted
// into a class anyway.
#include "nanopb_wrapper.cpp"

#include "crc_lib.h"

#include "server.h"
#include "client.h"

unsigned int g_fail_count = 0; // Global crc error counter

/*---------------------------------------------------------------------------*/
/* Server encode / decode helper functions                                   */

/* Read fields from a populated fwip_s struct and encode them through nanopb.
 * For each encoded field, store the result into a uint8_t buffer.
 * The uint8_t array has to be at least the maximum size of a fully encoded
 * payload, which can be accessed through the header file emmitted by nanopb.
 * Return false on error. */
bool encode_fwip(fwip_s * fwip, uint8_t * out)
{
    pb_ostream_t ostream = pb_ostream_from_buffer( out, gridware_FirmwareImagePage_size );

    if( ! encode( fwip->page, gridware_FirmwareImagePage_page_tag, PB_WT_STRING, &ostream, sizeof(fwip->page) ) )
    {
        return false;
    }

    if( ! encode( fwip->crc, gridware_FirmwareImagePage_crc_tag, PB_WT_STRING, &ostream, sizeof(fwip->crc) ) )
    {
        return false;
    }
        
    if( ! encode( fwip->last, gridware_FirmwareImagePage_last_tag, PB_WT_VARINT, &ostream ) )
    {
        return false;
    }

    return true;
}

/* Zero-set the destination uint8_t buffer and encode the provided fwip_s
 * struct into it. Report and return false on error. */
bool send_fwip( fwip_s * fwip, uint8_t * buffer )
{
    memset( buffer, 0, gridware_FirmwareImagePage_size );
    if( ! encode_fwip( fwip, buffer ))
    {
        printf( "encode_fwip failed.\n" );
        return false;
    }
    return true; 
}

/* Decode the device response from a buffer containing the pb payload. Write
 * the decoded gridware_DeviceResponse_verified_tag value into a boolean.
 * Return false on error recognizing the tag, or if decoding the content
 * fails. */
bool decode_dr( bool * verified, uint8_t * in )
{
    pb_istream_t istream = pb_istream_from_buffer( in, gridware_DeviceResponse_size );
    bool eof = 0;
    pb_wire_type_t wire;
    uint32_t tag;

    while( pb_decode_tag( &istream, &wire, &tag, &eof ) && istream.bytes_left > 0 )
    {
        // TODO: Check result of pb_decode_tag
        switch(tag)
        {
            case gridware_DeviceResponse_verified_tag:
                if( ! decode( verified, PB_WT_VARINT, &istream ) )
                {
                    return false;
                }
                break;

            default:
                printf( "decode_dr Unknown tag encountered %#x\n", tag );
                return false;
        }
    }
    return true;
}

/*---------------------------------------------------------------------------*/
/* Server firmware page processing loop                                      */

/* Process a given page, which is a uint8_t buffer, and evaluate the result 
 * returned by the client. */
bool server( internet_s * internet, uint8_t * page )
{
    fwip_s fwip = { .last = internet->last }; // TODO: Maybe don't set .last
    bool verified = false;
    bool client_response_available = false;
    uint16_t crc_uint16;
    uint8_t crc_arr[ 2 ];

    do {
        /* Wait for the client to release mutex before proceeding.
         * In real life this would be a SW interript. */
        while( internet->owner != SERVER ) { };
        
        /* Skip decoding if a client response isn't available. */
        if( client_response_available )
        {
            decode_dr( &verified, internet->buffer );
            printf( "SERVER : verified     = %#x\n", verified);
            if( verified ) client_response_available = false;
            // TODO: Finish implementing 
        }

        /* Calculate uin16_t CRC and convert it into a 2-byte uint8_t array. */
        crc_uint16 = crc16( 0, page, SZ_FW_PAGE );
        uint16_to_arr( crc_uint16, crc_arr );

        /* Copy the crc array and fw page into the fwip_s struct.  */
        memcpy( fwip.crc, crc_arr, 2 );
        memcpy( fwip.page, page, SZ_FW_PAGE );

        printf( "SERVER : encoding fwip:\n" );
        printf( "SERVER : fwip->crc    = %#x\n", crc_uint16);
        printf( "SERVER : fwip->last   = %#x\n", fwip.last);
        printf( "\n" );

        send_fwip( &fwip, internet->buffer );

    } while( client_response_available );

    /* Nothing else to do, so return ownership of the internet to client. */
    internet->owner = CLIENT;
    return true;
}


/*---------------------------------------------------------------------------*/
/* Main                                                                      */

/* Instantiate the internet and spawn the client as a thread with a pointer 
 * to the internet. Allocate some space for the firmware blob and populate it
 * with random uint8 bytes. Finally, send SZ_FW_PAGE of firmware at a time to
 * the server. */
int main( )
{
    internet_s internet;
    std::thread client_thread( client, &internet );

    /* Generate FW blob */
    uint8_t * fw_binary = (uint8_t *)malloc( SZ_FW ); // For server use only
    if ( fw_binary == NULL )
    {
        printf("Failed to malloc fw blob.\n");
        return 1;
    }
    srand (time(NULL));
    for( unsigned int i = 0; i < SZ_FW ; i++ ) {
        fw_binary[ i ] = rand() % 255;
    }

    /* Divide fw_binary into SZ_FW_PAGE byte slices, then send each slice until
     * SZ_FW bytes have been processed. Note that i goes a byte past SZ_FW
     * before the loop terminates. */
    uint8_t page[ SZ_FW_PAGE ];
    unsigned int i;
    for( i = 0; i <= SZ_FW ; i++ ) // TODO: Maybe convert to while / do loop
    { 
        if( ! ( i % SZ_FW_PAGE ) && ( i != 0 ) ) // TODO: This assumes that SZ_FW is divisable by NUM_FW_PAGE.
        {
            internet.last = ! ( SZ_FW - i );

            /* Note that the server evaluates the client response, in order to
             * resend on crc error without re-writing the page buffer. */
            server( &internet, page );
        }
        if( i != SZ_FW )
        {
            page[ i % SZ_FW_PAGE ] = fw_binary[ i ];
        }
    }

    /* All of fw_binary has been sliced and sent. Clean up. */
    free( fw_binary );
    client_thread.join();
    printf( "\nSERVER : Exiting. Sent ~ %u kb. Received %u crc failures.\n", i / 1024, g_fail_count );

    return 0;
}