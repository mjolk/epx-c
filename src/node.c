/**
 * File   : src/node.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 21:23
 */

#include <stdint.h>

void on_timeout()

typedef struct timer {
    int time_out;
    int elapsed;
    uint8_t paused;
} timer;
