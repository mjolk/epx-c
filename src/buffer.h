/**
 * File   : buffer.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 04:38
 */

#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

typedef struct cbuf cbuf;


cbuf* buf_init();
void buf_enqueue(cbuf* buf, void *data);
void* buf_dequeue(cbuf* buf);
