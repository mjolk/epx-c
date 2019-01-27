/**
 * File   : fbs_sock.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : wo 16 jan 2019 02:38
 */

#include <stdint.h>
#include "fbs/epx_builder.h"
#include <libdillimpl.h>

struct message;

typedef int (*decoder)(void*, void*);
typedef void (*encoder)(void*, flatcc_builder_t*);

struct fbs_sock {
    struct hvfs hvfs;
    struct msock_vfs mvfs;
    flatcc_builder_t b;
    int u;
    size_t hdrlen;
    unsigned int bigendian : 1;
    int deadline;
    int senderr;
    int recverr;
    int senddone;
    int recvdone;
    size_t replica_id;
    size_t xreplica_id;
    decoder decode;
    encoder encode;
};

int fbs_sock_attach(int, struct fbs_sock*);
int fbs_sock_detach(int, int64_t);
size_t fbs_sock_remote_id(int);
