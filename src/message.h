/**
 * File   : src/message.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : wo 12 sep 2018 16:06
 */

#include "command.h"

enum state {
    NONE,
    PRE_ACCEPTED,
    PRE_ACCEPTED_EQ,
    ACCEPTED,
    PREPARING,
    COMMITTED,
    EXECUTED
} state;


enum message_type {
    NACK,
    PHASE1,
    PRE_ACCEPT,
    PRE_ACCEPT_OK,
    PRE_ACCEPT_REPLY,
    ACCEPT,
    ACCEPT_REPLY,
    COMMIT,
    PREPARE,
    PREPARE_REPLY,
    TRY_PRE_ACCEPT,
    TRY_PRE_ACCEPT_REPLY
};

struct message {
    enum message_type type;
    size_t to;
    uint8_t ballot;
    struct instance_id id;
    struct command *command;
    uint64_t seq;
    struct dependency* deps[N];
    size_t from;
    enum state instance_status;
};

int message_from_buffer(struct message*, void*);
void message_to_buffer(struct message*, flatcc_builder_t*);
