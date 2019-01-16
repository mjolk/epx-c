/**
 * File   : src/buffer.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 04:38
 */

#include "buffer.h"
#include <stdio.h>
#include <errno.h>

int chan_init(struct chan *b) {
    int err = 0;
    b->read = 0;
    b->write = 0;
    b->capacity = 256;
    err = sem_init(&b->c_sem, 0, 0);
    if(err < 0) goto error;
    err = sem_init(&b->s_sem, 0, 256);
    if(err < 0) goto error;
    err = pthread_mutex_init(&b->lock, NULL);
    if(err < 0) goto error;
    return err;
error:
    perror("error....");
    return -1;
}

void send(struct chan* b, void *data){
    int err = 0;
    do
        err = sem_wait(&b->s_sem);
    while(err < 0 && errno == EINTR);
    if(err < 0) goto error;
    err = pthread_mutex_lock(&b->lock);
    if(err < 0) goto error;
    b->buffer[(b->write++)&(b->capacity -1)] = data;
    err = pthread_mutex_unlock(&b->lock);
    if(err < 0) goto error;
    err = sem_post(&b->c_sem);
    if(err < 0) goto error;
error:
    perror("error....");
}

void* recv(struct chan* b){
    int err = 0;
    void *result = NULL;
    do
        err = sem_wait(&b->c_sem);
    while(err < 0 && errno == EINTR);
    if(err < 0) goto error;
    err = pthread_mutex_lock(&b->lock);
    if(err < 0) goto error;
    result = b->buffer[(b->read++)&(b->capacity -1)];
    err = pthread_mutex_unlock(&b->lock);
    if(err < 0) goto error;
    err = sem_post(&b->s_sem);
    if(err < 0) goto error;
    return result;
error:
    perror("error....");
    return 0;
}
