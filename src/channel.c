/**
 * File   : channel.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : vr 08 feb 2019 19:19
 */
#include "channel.h"

void chan_init(chan *c){
   ck_ring_init(&c->ring, CAPACITY);
}

int chan_send_spmc(chan *c, void *data){
     return ck_ring_enqueue_spmc(&c->ring, c->b, data);
}

int chan_recv_spmc(chan *c, void *data){
    return ck_ring_dequeue_spmc(&c->ring, c->b, &data);
}

int chan_send_mpsc(chan *c, void *data){
    return ck_ring_enqueue_mpsc(&c->ring, c->b, data);
}

int chan_recv_mpsc(chan *c, void *data){
    return ck_ring_dequeue_mpsc(&c->ring, c->b, &data);
}
