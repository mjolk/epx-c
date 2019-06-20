/**
 * File   : node.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 20:51
 */

#include <pthread.h>
#include "io.h"
#include "epaxos.h"

struct node {
    struct node_io io;
    struct replica r;
    pthread_t executor;
    pthread_t epaxos;
    char nodes[N];
};

int start(struct node*);
void stop(struct node*);