/**
 * File   : channel.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 04:38
 */

#include "channel.h"
#include <stdio.h>
#include <errno.h>

int chan_init(chan *b) {
    b->read = 0;
    b->write = 0;
    b->capacity = CAPACITY;
    if(sem_init(&b->c_sem, 0, 0) != 0) goto init_error;
    if(sem_init(&b->s_sem, 0, b->capacity) != 0) goto init_error;
    if(pthread_mutex_init(&b->lock, NULL) != 0) goto init_error;
    return 0;
init_error:
    perror("sync error");
    return -1;
}

void send(chan* b, void *data){
    int err = 0;
    do
        err = sem_wait(&b->s_sem);
    while(err != 0 && errno == EINTR);
    if(err != 0 && errno != EINTR) goto send_error;
    if(pthread_mutex_lock(&b->lock) != 0) goto send_error;
    b->buffer[(b->write++)&(b->capacity -1)] = data;
    if(pthread_mutex_unlock(&b->lock) != 0) goto send_error;
    if(sem_post(&b->c_sem) != 0) goto send_error;
    return;
send_error:
    perror("send sync error");
    return;
}

void* recv(chan* b){
    int err = 0;
    void *result = NULL;
    do
        err = sem_wait(&b->c_sem);
    while(err != 0 && errno == EINTR);
    if(err != 0 && errno != EINTR) goto recv_error;
    if(pthread_mutex_lock(&b->lock) != 0) goto recv_error;
    result = b->buffer[(b->read++)&(b->capacity -1)];
    if(pthread_mutex_unlock(&b->lock) != 0) goto recv_error;
    if(sem_post(&b->s_sem) != 0) goto recv_error;
    return result;
recv_error:
    perror("recv sync error");
    return 0;
}

int chan_size(chan *b){
    return b->write - b->read;
}
