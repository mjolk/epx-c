/**
 * File   : buffer.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 04:38
 */

#include "buffer.h"
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <errno.h>

typedef struct cbuf {
    uint32_t read;
    uint32_t write;
    void* buffer[256];
    int capacity;
    pthread_mutex_t lock;
    sem_t c_sem;
    sem_t s_sem;
} cbuf;

cbuf* buf_init() {
    int err = 0;
    cbuf* b;
    b = malloc(sizeof(cbuf));
    if(!b){goto error;}
    assert(b);
    b->read = 0;
    b->write = 0;
    b->capacity = 256;
    err = sem_init(&b->c_sem, 0, 0);
    if(err < 0){goto error;}
    err = sem_init(&b->s_sem, 0, 256);
    if(err < 0){goto error;}
    err = pthread_mutex_init(&b->lock, NULL);
    if(err < 0){goto error;}
error:
    perror("error....");
    return b;
}

void buf_enqueue(cbuf* b, void *data){
    int err = 0;
    do
        err = sem_wait(&b->s_sem);
    while(err < 0 && errno == EINTR);
    if(err < 0){goto error;}
    err = pthread_mutex_lock(&b->lock);
    if(err < 0){goto error;}
    b->buffer[(b->write++)&(b->capacity -1)] = data;
    err = pthread_mutex_unlock(&b->lock);
    if(err < 0){goto error;}
    err = sem_post(&b->c_sem);
    if(err < 0){goto error;}
error:
    perror("error....");
}

void* buf_dequeue(cbuf* b){
    int err = 0;
    void *result = NULL;
    do
        err = sem_wait(&b->c_sem);
    while(err < 0 && errno == EINTR);
    if(err < 0){goto error;}
    err = pthread_mutex_lock(&b->lock);
    if(err < 0){goto error;}
    result = b->buffer[(b->read++)&(b->capacity -1)];
    err = pthread_mutex_unlock(&b->lock);
    if(err < 0){goto error;}
    err = sem_post(&b->s_sem);
    if(err < 0){goto error;}
    return result;
error:
    perror("error....");
    return result;
}
