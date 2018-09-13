/**
 * File   : src/message.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : wo 12 sep 2018 16:06
 */

#include "fbs/epx_builder.h"

enum message_type {
    NACK,
    PRE_ACCEPT,
    PRE_ACCEPT_OK,
    PRE_ACCEPT_REPLY,
    ACCEPT,
    ACCEPT_OK,
    COMMIT,
    PREPARE,
    PREPARE_OK
};

struct message {
    enum message_type type;
    size_t to;
    uint8_t ballot;
    struct instance_id id;
    struct command *command;
    uint64_t seq;
    struct instance_id deps[8];
};

int message_from_buffer(struct message*, void*);
void message_to_buffer(struct message*, flatcc_builder_t*);
