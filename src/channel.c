/**
 * File   : channel.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : vr 08 feb 2019 19:19
 */
#include "channel.h"
#include <errno.h>
#include <stdio.h>

int chan_init(chan *c){
    ck_ring_init(&c->ring, CAPACITY);
    #ifdef __APPLE__
    c->c_sem = dispatch_semaphore_create(0);
    if(c->c_sem == NULL) goto init_error;
    c->s_sem = dispatch_semaphore_create(CAPACITY);
    if(c->s_sem == NULL) goto init_error;
    #else
    if(sem_init(&c->c_sem, 0, 0) != 0) goto init_error;
    if(sem_init(&c->s_sem, 0, CAPACITY) != 0) goto init_error;
    #endif
    return 0;
init_error:
    perror("init sync error");
    return -1;
}

int chan_send_spmc(chan *c, void *data){
    return ck_ring_enqueue_spmc(&c->ring, c->b, data);
}

int chan_recv_spmc(chan *c, void *data){
    return ck_ring_dequeue_spmc(&c->ring, c->b, data);
}

int chan_send_mpsc(chan *c, void *data){
    return ck_ring_enqueue_mpsc(&c->ring, c->b, data);
}

int chan_recv_mpsc(chan *c, void *data){
    return ck_ring_dequeue_mpsc(&c->ring, c->b, data);
}

int sem_chan_send_mpsc(chan *c, void *data){
    int rc = 0;
    #ifdef __APPLE__
    dispatch_semaphore_wait(c->s_sem, DISPATCH_TIME_FOREVER);
    #else
    do
        rc = sem_wait(&c->s_sem);
    while(rc != 0 && errno == EINTR);
    if(rc != 0 && errno != EINTR) goto send_error;
    #endif
    rc = ck_ring_enqueue_mpsc(&c->ring, c->b, data);
    #ifdef __APPLE__
    if(dispatch_semaphore_signal(c->c_sem) != 0) goto send_error;
    #else
    if(sem_post(&c->c_sem) != 0) goto send_error;
    #endif
    return rc;
send_error:
    perror("send sync error");
    return -1;
}

int sem_chan_recv_mpsc(chan *c, void *data){
    int rc = 0;
    #ifdef __APPLE__
    if(dispatch_semaphore_wait(c->c_sem, DISPATCH_TIME_FOREVER) != 0) goto recv_error;
    #else
    do
        rc = sem_wait(&c->c_sem);
    while(rc != 0 && errno == EINTR);
    if(rc != 0 && errno != EINTR) goto recv_error;
    #endif
    rc = ck_ring_dequeue_mpsc(&c->ring, c->b, data);
    #ifdef __APPLE__
    if(dispatch_semaphore_signal(c->s_sem) < 0) goto recv_error;
    #else
    if(sem_post(&c->s_sem) != 0) goto recv_error;
    #endif
    return rc;
recv_error:
    perror("recv sync error");
    return -1;
}

int chan_send_spsc(chan *c, void *data){
    return ck_ring_enqueue_spsc(&c->ring, c->b, data);
}

int chan_recv_spsc(chan *c, void *data){
    return ck_ring_dequeue_spsc(&c->ring, c->b, data);
}

int chan_send_mpmc(chan *c, void *data){
    return ck_ring_enqueue_mpmc(&c->ring, c->b, data);
}

int chan_recv_mpmc(chan *c, void *data){
    return ck_ring_dequeue_mpmc(&c->ring, c->b, data);
}
