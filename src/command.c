/**
 * File   : src/command.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail->com>
 * Date   : di 11 sep 2018 23:26
 */

#include "command.h"
#include "fbs/flatbuffers_common_reader.h"

void command_from_buffer(struct command *c, const void *buffer) {
    ns(command_table_t) command = ns(instance_data_command(buffer));
    c->id = ns(command_id(command));
    flatbuffers_string_t start = ns(command_start_key(command));
    //TODO defend overflow
    strncpy(c->span.start_key, start, flatbuffers_string_len(start));
    flatbuffers_string_t end = ns(command_end_key(command));
    //TODO defend overflow
    strncpy(c->span.end_key, end, flatbuffers_string_len(end));
    c->writing = ns(command_writing(command));
    //TODO defend buffer overflow
    size_t scp = sizeof(ns(command_value(command)));
    memcpy(&c->value, ns(command_value(command)), scp);
}

void command_to_buffer(struct command *c, flatcc_builder_t *b) {
    ns(command_start(b));
    ns(command_id_add(b, c->id));
    ns(command_start_key_create_str(b, c->span.start_key));
    ns(command_end_key_create_str(b, c->span.end_key));
    ns(command_writing_add(b, c->writing));
    ns(command_value_create(b, c->value,
                sizeof(c->value)));
    ns(command_end(b));
}

struct command* new_command() {
    struct command *c;
    c = malloc(sizeof(struct command));
    if(c == 0) return 0;
    return c;
}

int overlaps(struct span *s1, struct span *s2){
    return (strncmp(s1->start_key, s2->end_key, KEY_SIZE) <= 0)&& (strncmp(s1->end_key, s2->start_key, KEY_SIZE) >= 0);
}

int encloses(struct span *s1, struct span *s2){
    return (strncmp(s1->start_key, s2->start_key, KEY_SIZE) <= 0) && (strncmp(s1->end_key, s2->end_key, KEY_SIZE) >= 0);
}

int interferes(struct command *c1, struct command *c2) {
    return (c1->writing || c2->writing) && overlaps(&c1->span, &c2->span);
}
