/**
 * File   : node_sync.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : zo 10 feb 2019 13:18
 */

#ifndef EPX_SYNC
#define EPX_SYNC
#include "../include/epx.h"
#include "channel.h"

enum connection_status {
    DEAD,
    CONNECTING,
    ALIVE
};

//TO replica 
struct replica_sync {
    chan chan_step;//messages from other replicas
    chan chan_propose;//commands from clients
    chan chan_ctl;//control commands
};

//FROM replica
struct io_sync {
    chan chan_eo;//events for clients(external out) on commit
    chan chan_io;//internal node communications(internal out)
    chan chan_exec;//instances ready to be executed
};
#endif