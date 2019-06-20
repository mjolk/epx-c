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
    if(ns(instance_data_command_is_present((ns(instance_data_table_t))buffer))){
        ns(command_table_t) cmd =
            ns(instance_data_command((ns(instance_data_table_t))buffer));
        if(cmd == 0){
            return 1;
        }
        c->id = ns(command_id(cmd));
        ns(span_vec_t) spans = ns(command_spans(cmd));
        c->tx_size = ns(span_vec_len(spans));
        for(size_t i = 0;i < c->tx_size;i++){
            strcpy(c->spans[i].start_key,
                    ns(span_start_key(ns(span_vec_at(spans, i)))));
            strcpy(c->spans[i].end_key,
                    ns(span_end_key(ns(span_vec_at(spans, i)))));
        }
        c->writing = (enum io_t)ns(command_writing(cmd));
        //TODO defend buffer overflow
        //size_t scp = sizeof(ns(command_value(cmd)));
        //memcpy(&c->value, ns(command_value(cmd)), scp);
        return 0;
    }
    return 1;
}

epx_command_ref_t command_to_buffer(struct command *c, flatcc_builder_t *b) {
    ns(command_start(b));
    ns(command_id_add(b, c->id));
    ns(command_spans_start(b));
    for(int i = 0;i < TX_SIZE;i++){
        if(empty_range(&c->spans[i])) continue;
        ns(command_spans_push_create(b,
                    flatbuffers_string_create_str(b, c->spans[i].start_key),
                    flatbuffers_string_create_str(b, c->spans[i].end_key)
                    ));
    }
    ns(command_spans_end(b));
    ns(command_writing_add(b, c->writing));
    /* while(*c->value){
        ns(command_value_push_create(b,
            flatbuffers_char_vec_create(b, *c->value, 
                sizeof(*c->value)/sizeof(*c->value[0]))));
        c->value++;
    } */
    return ns(command_end(b));
}

int overlaps(struct span *s1, struct span *s2){
    int ret = (strcmp(s1->start_key, s2->end_key) <= 0) &&
        (strcmp(s1->end_key, s2->start_key) >= 0);
    return ret;
}

int empty_range(struct span *s){
    return (s->start_key[0] == '\0');
}

//brute force, can't sort need to copy/preserve/save to maintain order.
int overlap_sparse(struct command *c1, struct command *c2, struct span *c){
    int ret = 0;
    int cnt = 0;
    for(size_t i = 0;i < c1->tx_size;i++){
        for(size_t j = 0;j < c2->tx_size;j++){
            if(overlaps(&c1->spans[i], &c2->spans[j])){
                c[cnt] = c1->spans[i];
                ret = 1;
                cnt++;
            }
        }
    }
    return ret;
}

int encloses(struct span *s1, struct span *s2){
    return (strcmp(s1->start_key, s2->start_key) <= 0) &&
        (strcmp(s1->end_key, s2->end_key) >= 0);
}

int interferes(struct command *c1, struct command *c2, struct span *c) {
    return (c1->writing || c2->writing) && overlap_sparse(c1, c2, c);
}
