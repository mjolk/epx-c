/**
 * File   : channel.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : vr 08 feb 2019 19:18
 */

#include <semaphore.h>
#include <ck_ring.h>
#define CAPACITY 2048

typedef struct chan {
    ck_ring_t ring;
    ck_ring_buffer_t b[CAPACITY];
    sem_t c_sem;
    sem_t s_sem;
} chan;

int chan_init(chan*);
int chan_send_spmc(chan*, void*);
int chan_recv_spmc(chan*, void*);
int chan_send_mpsc(chan*, void*);
int chan_recv_mpsc(chan*, void*);
int chan_send_spsc(chan*, void*);
int chan_recv_spsc(chan*, void*);
int chan_send_mpmc(chan*, void*);
int chan_recv_mpmc(chan*, void*);
int sem_chan_send_mpsc(chan*, void*);
int sem_chan_recv_mpsc(chan*, void*);
