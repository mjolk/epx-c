/**
 * File   : node_io_io.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : zo 10 feb 2019 13:29
 */

#include <pthread.h>
#include "fbs_sock.h"
#include "node_sync.h"

enum protocol {
    TCP,
    TLS,
    UDP
};

struct connection {
    size_t id;
    int handle;
    struct ipaddr addr;
    uint64_t latency;
    int back_off;
    int error;
    enum protocol p;
    struct fbs_sock fbs;
    int ap;
    int sending;
    int alive;
    pthread_mutex_t lock;
};

struct node_io {
    int running;
    int ap;
    struct sync *sync;
    size_t quorum[N];
    struct connection cons[N];
};

int start(struct node_io*);
void stop(struct node_io*);
int read_int(struct node_io*, struct message*);
int write_int(struct node_io*, struct message*);
int read_ext(struct node_io*, struct message*);
int write_ext(struct node_io*, struct message*);
int report(int, int*);
