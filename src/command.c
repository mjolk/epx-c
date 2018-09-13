/**
 * File   : src/message.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : di 11 sep 2018 23:26
 */

#include "command.h"
#include "fbs/epx_reader.h"

#undef ns
#define ns(x) FLATBUFFERS_WRAP_NAMESPACE(epx, x) // Specified in the schema.

void command_from_buffer(struct command *c, const void *buffer) {
    ns(command_table_t) command = ns(instance_data_command(buffer));
    c->id = ns(command_id(command));
    ns(span_struct_t) span = ns(command_span(command));
    c->span.start = ns(span_start(span));
    c->span.end = ns(span_end(span));
    c->writing = ns(command_writing(command));
    //TODO defend buffer overflow
    size_t scp = sizeof(ns(command_value(command)));
    memcpy(&c->value, ns(command_value(command)), scp);
}

void command_to_buffer(struct command *c, flatcc_builder_t *b) {
    ns(command_start(b));
    ns(command_id_add(b, c->id));
    ns(command_span_create(b, c->span.start,
                c->span.end));
    ns(command_writing_add(b, c->writing));
    ns(command_value_create(b, c->value,
                sizeof(c->value)/sizeof(c->value[0])));
    ns(command_end(b));
}
