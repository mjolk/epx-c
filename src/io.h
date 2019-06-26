/**
 * File   : node_io_io.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : zo 10 feb 2019 13:29
 */

#ifndef EPX_IO
#define EPX_IO
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

struct nxt_sp {
    struct registered_span *sle_next;
};

struct client {
    int chan[2];
    int64_t expires;
};

struct connection {
    enum connection_status status;
    size_t xreplica_id;
    int handle;
    int prot_handle;
    pthread_t tid;
    struct node_io *n;
    struct client client;
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
};

struct registered_span {
    char start_key[KEY_SIZE];
    char end_key[KEY_SIZE];
    char max[KEY_SIZE];
    struct nxt_sp next;
    struct client clients[MAX_CLIENTS];
    LLRB_ENTRY(registered_span) entry;
};

LLRB_HEAD(client_index, registered_span);
LLRB_PROTOTYPE(client_index, registered_span, entry, spcmp)

struct node_io {
    size_t node_id;
    int node_listener;
    int client_listener;
    pthread_attr_t tattr;
    pthread_t c_tid;
    volatile int running;
    int ap;
    int ap_client;
    struct replica_sync sync;
    struct io_sync io_sync;
    struct connection chan_nodes[N];
    size_t quorum[N];
    struct client_index clients;
};

int start_io(struct node_io*);
void stop_io(struct node_io*);
int node_connect(struct node_io*, struct ipaddr*, int fd);
#endif