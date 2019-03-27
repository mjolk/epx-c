/**
 * File   : node_sync.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : zo 10 feb 2019 13:18
 */

#include "../include/epx.h"
#include "channel.h"

enum connection_status {
    ALIVE,
    CONNECTING,
    DEAD
};

struct replica_sync {
    chan chan_exec;//OUT instances ready to be executed
    chan chan_step;//IN messages from other replicas
    chan chan_propose;//IN messages from clients
};

struct io_sync {
    chan chan_eo;//OUT events for clients(external out)
    chan chan_io;//OUT internal node communications(internal out)
};
