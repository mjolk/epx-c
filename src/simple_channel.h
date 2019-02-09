/**
 * File   : simple_channel.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 04:38
 */

#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define CAPACITY 2048

typedef struct simple_chan {
    uint32_t read;
    uint32_t write;
    void* buffer[CAPACITY];
    int capacity;
    pthread_mutex_t lock;
    sem_t c_sem;
    sem_t s_sem;
} simple_chan;

int simple_chan_init(simple_chan*);
void simple_chan_send(simple_chan*, void*);
void* simple_chan_recv(simple_chan*);
int simple_chan_size(simple_chan*);
