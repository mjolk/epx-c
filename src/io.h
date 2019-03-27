/**
 * File   : node_io_io.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : zo 10 feb 2019 13:29
 */

#include <pthread.h>
#include "fbs_sock.h"
#include "sync.h"
#include "message.h"
#include "llrb-interval/llrb.h"

enum protocol {
    TCP,
    TLS,
    UDP
};

struct connection {
    volatile enum connection_status status;
    size_t xreplica_id;
    int handle;
    pthread_t *tid;
    struct node_io *n;
    int fd;
    uint64_t latency;
    struct ipaddr addr;
    int back_off;
    int error;
    enum protocol p;
    struct fbs_sock fbs;
    int ap;
    chan chan_read;
    chan *chan_write;
    pthread_rwlock_t lock;
};

struct client {
    int handle;
    int64_t expires;
};

struct registered_span {
    char start_key[KEY_SIZE];
    char end_key[KEY_SIZE];
    char max[KEY_SIZE];
    struct nxt next;
    struct client clients[10];
    LLRB_ENTRY(registered_span) entry;
};

LLRB_HEAD(client_tree, registered_span);
LLRB_PROTOTYPE(client_tree, registered_span, entry, spcmp)

struct node_io {
    size_t node_id;
    int running;
    int ap;
    struct replica_sync sync[1];
    struct io_sync io_sync;
    chan chan_step;//IN steps for replica
    chan chan_propose;//IN proposals for replica
    chan chan_ctl;//IN ctl commands
    struct connection chan_nodes[N];
    struct span ranges[1];
    size_t quorum[N];
    struct client_tree clients;
};

int start(struct node_io*);
void stop(struct node_io*);
