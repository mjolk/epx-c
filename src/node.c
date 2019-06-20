/**
 * File   : node.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 21:23
 */

#include "node.h"
#include "execute.h"

struct node* new_node(size_t id, char *nodes){
    struct node *n = (struct node*)calloc(1, sizeof(struct node));
    if(!n) return 0;
    n->io.node_id = id;
    n->r.id = id;
    n->r.out = &n->io.io_sync;
    n->r.in = &n->io.sync;
    struct ipaddr addr;
    for(size_t i = 0;i < N;i++){
        if(i == id){
            ipaddr_local(&addr, "127.0.0.1", NODE_PORT, 0);
            n->io.nodes[i] = addr;
            continue;
        }
        ipaddr_remote(&addr, &nodes[i], NODE_PORT, 0, -1);
        n->io.nodes[i] = addr;
    }
    return n;
}

int start(struct node *n){
    pthread_attr_t detached;
    pthread_attr_init(&detached);
    pthread_attr_setdetachstate(&detached, PTHREAD_CREATE_DETACHED);
    if(pthread_create(&n->epaxos, NULL, run, &n->r) != 0) return -1;
    if(pthread_create(&n->executor, &detached, run_execute, &n->r) != 0){
        pthread_cancel(n->epaxos);
        return -1;
    }
    if(start_io(&n->io) != 0) goto error;
    if(pthread_join(n->epaxos, NULL) != 0) goto error;
    return 0;
error:
    pthread_cancel(n->epaxos);
    pthread_cancel(n->executor);
    return -1;
}

void stop(struct node *n){
    stop_io(&n->io);
}
