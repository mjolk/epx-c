/**
 * File   : node_sync.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : zo 10 feb 2019 13:18
 */

#include "../include/epx.h"
#include "channel.h"

struct sync {
    int l_fd;
    int r_fd[N];
    chan chan_io;
    chan chan_exec;
    chan chan_eo;
    chan chan_cont;
    chan chan_new;
};
