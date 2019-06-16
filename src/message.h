/**
 * File   : message.h
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : wo 12 sep 2018 16:06
 */

#include "command.h"

enum state {
    NONE = 0x01,
    PRE_ACCEPTED = 0x02,
    PRE_ACCEPTED_EQ = 0x04,
    ACCEPTED = 0x08,
    PREPARING = 0x10,
    TRY_PRE_ACCEPTING = 0x20,
    COMMITTED = 0x40,
    EXECUTED = 0x80
} state;


enum message_type {
    PHASE1 = 1,
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
    uint8_t nack;
    struct instance_id id;
    struct command *command;
    uint64_t seq;
    struct dependency deps[MAX_DEPS];
    size_t from;
    enum state instance_status;
    struct {
        struct instance_id id;
        enum state status;
    } conflict;
    uint64_t start;
    uint64_t stop;
};

#define MSG_SIZE sizeof(struct message*)

int message_from_buffer(void*, void*);
void message_to_buffer(void*, flatcc_builder_t*);
struct message* copy_message(struct message*);