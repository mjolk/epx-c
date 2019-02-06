/**
 * File   : channel.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 04:38
 */

#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define CAPACITY 2048

typedef struct chan {
    uint32_t read;
    uint32_t write;
    void* buffer[CAPACITY];
    int capacity;
    pthread_mutex_t lock;
    sem_t c_sem;
    sem_t s_sem;
} chan;

int chan_init(chan*);
void chan_send(chan* buf, void *data);
void* chan_recv(chan* buf);
int chan_size(chan*);