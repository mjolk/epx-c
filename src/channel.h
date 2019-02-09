/**
 * File   : channel.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : vr 08 feb 2019 19:18
 */

#include <ck_ring.h>
#define CAPACITY 2048

typedef struct chan {
    ck_ring_t ring;
    ck_ring_buffer_t b[CAPACITY];
} chan;

void chan_init(chan*);
int chan_send_spmc(chan*, void*);
int chan_recv_spmc(chan*, void*);
int chan_send_mpsc(chan*, void*);
int chan_recv_mpsc(chan*, void*);
