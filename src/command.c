/**
 * File   : command.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail->com>
 * Date   : di 11 sep 2018 23:26
 */

#include <stdio.h>
#include "command.h"
#include "fbs/flatbuffers_common_reader.h"


int command_from_buffer(struct command *c, const void *buffer) {
    if(ns(instance_data_command_is_present(buffer))){
        ns(command_table_t) cmd = ns(instance_data_command(buffer));
        if(cmd == 0){
            return 1;
        }
        c->id = ns(command_id(cmd));
        ns(span_vec_t) spans = ns(command_spans(cmd));
        size_t sln = ns(span_vec_len(spans));
        for(size_t i = 0;i < sln;i++){
            strcpy(c->spans[i].start_key,
                    ns(span_start_key(ns(span_vec_at(spans, i)))));
            strcpy(c->spans[i].end_key,
                    ns(span_end_key(ns(span_vec_at(spans, i)))));
        }
        c->writing = ns(command_writing(cmd));
        //TODO defend buffer overflow
        size_t scp = sizeof(ns(command_value(cmd)));
        //memcpy(&c->value, ns(command_value(cmd)), scp);
        return 0;
    }
    return 1;
}

epx_command_ref_t command_to_buffer(struct command *c, flatcc_builder_t *b) {
    ns(command_start(b));
    ns(command_id_add(b, c->id));
    ns(command_spans_start(b));
    for(int i = 0;i < 100;i++){
        if(c->spans[i].start_key[0] == '\0') continue;
        ns(command_spans_push_create(b,
                    flatbuffers_string_create_str(b, c->spans[i].start_key),
                    flatbuffers_string_create_str(b, c->spans[i].end_key)
                    ));
    }
    ns(command_spans_end(b));
    ns(command_writing_add(b, c->writing));
    if(c->value){
        ns(command_value_create(b, c->value,
                    sizeof(c->value)));
    }
    return ns(command_end(b));
}

int overlaps(struct span *s1, struct span *s2){
    int ret = (strcmp(s1->start_key, s2->end_key) <= 0) &&
        (strcmp(s1->end_key, s2->start_key) >= 0);
    return ret;
}

int encloses(struct span *s1, struct span *s2){
    return (strcmp(s1->start_key, s2->start_key) <= 0) &&
        (strcmp(s1->end_key, s2->end_key) >= 0);
}

int interferes(struct command *c1, struct command *c2) {
    return (c1->writing || c2->writing) && overlaps(&c1->spans, &c2->spans);
}
