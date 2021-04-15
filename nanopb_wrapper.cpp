#include <cstdio>
#include <pb_encode.h>
#include <pb_decode.h>
#include <pb_common.h>

/* Takes in an arbitrary type, and encodes it into ostream. The
 * respective pb_encoder is call depending on the wire_type. */
template<typename T>
bool encode( T in, uint32_t tag, pb_wire_type_t wire_type, pb_ostream_t * ostream, size_t bufsize = 0 )
{
    if( ! pb_encode_tag( ostream, wire_type, tag ) )
    {
        printf( "encode failed: pb_encode_tag failed for tag %#x and wire_type %#x: %s\n", tag, wire_type, PB_GET_ERROR(ostream) );
        return false;
    }
    switch( wire_type )
    {
        case PB_WT_VARINT:
            if( ! pb_encode_varint( ostream, (uint64_t)in ) )
            {
                printf( "encode failed: pb_encode_varint failed for tag %#x and wire_type %#x: %s\n", tag, wire_type, PB_GET_ERROR(ostream) );
                return false;
            }
            break;

        case PB_WT_STRING:
            if ( ! bufsize )
            {
                printf( "encode failed: buffer size needs to be specified for byte arrays (tag %#x).\n", tag );
                return false;
            }
            if( ! pb_encode_string( ostream, (pb_byte_t *)(uintptr_t)in, bufsize ) )
            {
                printf( "encode failed: pb_encode_string failed for tag %#x and wire_type %#x: %s\n", tag, wire_type, PB_GET_ERROR(ostream) );
                return false;
            }
            break;

        default:
            printf( "encode failed: unrecognized pb_wire_type %#x.\n", wire_type );
            return false;
    }
    return true;
}

/* Decodes a buffer containing pb data, and decodes it to a type T depending on
 * the wire_type. */
template<typename T>
bool decode( T out, pb_wire_type_t wire_type, pb_istream_t * istream, size_t bufsize = 0 )
{
    switch (wire_type)
    {
        case PB_WT_VARINT:
            if( ! pb_decode_varint32( istream, (uint32_t*)out ) )
            {
                printf( "decode failed: pb_decode error for wire_type %#x: %s\n", wire_type, PB_GET_ERROR( istream ) );
                return false;
            }
            break;

        case PB_WT_STRING:
            if ( ! bufsize )
            {
                printf( "decode failed: buffer size needs to be specified for byte arrays (wire_type %#x).\n", wire_type );
                return false;
            }
            pb_istream_t substream;
            if( ! pb_make_string_substream( istream, &substream ) )
            {
                printf( "decode failed: pb_make_string_substream failed for wire_type %#x: (substream) %s, (istream) %s\n", wire_type, PB_GET_ERROR( &substream ), PB_GET_ERROR( istream ) );
                return false;
            }
            if( bufsize < substream.bytes_left )
            {
                printf( "decode failed: specified destination buffer size %lu is too small to decode the substream of %lu bytes (wire_type %#x).\n", bufsize, substream.bytes_left, wire_type );
                return false;
            }
            // Casting because this logic only hit when out is of type XXX
            pb_read( &substream, (pb_byte_t *)out, substream.bytes_left );
            pb_close_string_substream( istream, &substream );
            break;

        default:
            printf( "decode failed: unrecognized pb_wire_type %u.\n", wire_type );
            return false;
    }
    return true;
}