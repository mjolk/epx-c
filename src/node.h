/**
 * File   : node.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 20:51
 */

#include "epaxos.h"
#include "channel.h"
#include "fbs_sock.h"
#include <pthread.h>

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

struct node {
    chan chan_eo;
    chan chan_ei;
    chan chan_io;
    chan chan_ii;
    chan chan_exec;
    struct replica r;
    int running;
    int amplitude;
    int ap;
    size_t quorum[N];
    struct connection cons[N];
};

int start(struct node*);
void stop(struct node*);
struct message *read_int(struct node*);
void write_int(struct node*, struct message*);
struct message *read_ext(struct node*);
void write_ext(struct node*, struct message*);
int report(int, int*);
