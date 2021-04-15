#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include "fw_update.pb.h"

// Tunables
#define SZ_FW_PAGE   2048
#define NUM_FW_PAGE  512
#define SZ_FW        SZ_FW_PAGE*NUM_FW_PAGE

/* Global crc failure count. Next time I'd fit this into the internet_s struct
 * to allow mutex'd usage of g_fail_count, but for now only the server
   increments it. */
extern unsigned int g_fail_count;

typedef enum {
    SERVER = 0,
    CLIENT = 1
} owner_mtx_t;

/*  internet_s is used as the global state for server <-> comms.
 *  It encodes a mutex that either of them can set, and functions using this
 *  struct are expected to check and set the mutex between accesses.
 *  Next time I'd just make a class with getters and setters to set the mutex. */
struct internet_s {
    /* Buffer used for populating / reading internet messages by either the
     * server or client.
     * Since gridware_FirmwareImagePage_size is the largest encoded payload,
     * we set the buffer size to that. */
    uint8_t buffer[ gridware_FirmwareImagePage_size ];

    /* The compiler won't be able to statically assert that the owner mtx has
     * effects throughout the lifetime of the multithreaded program, which
     * may cause it to be optimized out unless declared volatile. */
    volatile owner_mtx_t owner = SERVER;
    
    /* The server uses this to write (and check) whether the payload it's
     * about to send is the last one, in order to shut down the connection. */
    bool last;
};

/* Internal representation of FWIP used by server / client. */
struct fwip_s {
    uint8_t page[ SZ_FW_PAGE ];

    /* crc is an array as specified by the protobuf buffer. crc_lib provides
     * arr_to_uint16(...) to retreive the uint16 encoding of the crc.
     * Next time I'd make crc a uin16t first class citizen, since this struct
     * is intended to be a specialization of the protobuf fields. */
    uint8_t crc[ 2 ]; 
    bool last;
};

#endif