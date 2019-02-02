/**
 * File   : message.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : di 11 sep 2018 23:26
 */

#include "message.h"
#include <stdio.h>


void instance_data_from_buffer(struct message *m, const void *buffer){
    if(buffer == 0){
        return;
    }
    int cp = command_from_buffer(m->command, buffer);
    if(cp > 0 ){
        m->command = 0;
    }
    m->seq = ns(instance_data_seq(buffer));
    ns(dependency_vec_t) rdps = ns(instance_data_deps(buffer));
    size_t veclen = ns(dependency_vec_len(rdps));
    size_t len = (veclen > N)?N:veclen;
    for(size_t i = 0; i < len; i++){
        ns(dependency_struct_t) sid = ns(dependency_vec_at(rdps, i));
        m->deps[i].id.instance_id = ns(dependency_instance_id(sid));
        m->deps[i].id.replica_id = ns(dependency_replica_id(sid));
        m->deps[i].committed = ns(dependency_committed(sid));
    }
}

int message_from_buffer(void *im, void *buffer){
    struct message *m = im;
    ns(message_table_t) bm = ns(message_as_root(buffer));
    if(!bm) goto error;
    m->to = ns(message_to(bm));
    m->from = ns(message_from(bm));
    m->nack = ns(message_nack(bm));
    m->ballot = ns(message_ballot(bm));
    m->instance_status = ns(message_instance_status(bm));
    ns(instance_id_struct_t) i_id = ns(message_instance_id(bm));
    if(!i_id){
        goto error;
    }
    m->id.replica_id = ns(instance_id_replica_id(i_id));
    m->id.instance_id = ns(instance_id_instance_id(i_id));
    if(!ns(message_type_is_present(bm))){
        goto error;
    }
    m->type = ns(message_type(bm));
    if(ns(message_stp_is_present(bm))){
        m->stop = ns(message_stp(bm));
    }
    if(ns(message_srt_is_present(bm))){
        m->start = ns(message_srt(bm));
    }
    switch(m->type){
        case PRE_ACCEPT_OK:
            break;
        default:
            instance_data_from_buffer(m, ns(message_data(bm)));

    }
    return 0;
error:
    printf("error occurred....\n");
    return 1;
}

epx_instance_data_ref_t instance_data_to_buffer(struct message *m,
        flatcc_builder_t *b){
    ns(instance_data_start(b));
    if(m->command){
        ns(instance_data_command_add(b, command_to_buffer(m->command, b)));
    }
    ns(instance_data_seq_add(b, m->seq));
    ns(instance_data_deps_start(b));
    for(size_t i = 0; i < MAX_DEPS; i++){
        if(m->deps[i].id.replica_id > 0){
            ns(instance_data_deps_push_create(b,
                        m->deps[i].id.replica_id, m->deps[i].id.instance_id,
                        m->deps[i].committed));
        }
    }
    ns(instance_data_deps_add(b, ns(instance_data_deps_end(b))));
    return ns(instance_data_end(b));
}

void message_to_buffer(void *im, flatcc_builder_t *b){
    struct message *m = im; 
    ns(message_start_as_root(b));
    ns(message_to_add(b, m->to));
    ns(message_from_add(b, m->from));
    ns(message_nack_add(b, m->nack));
    ns(message_ballot_add(b, m->ballot));
    ns(message_instance_id_create(b, m->id.replica_id, m->id.instance_id));
    ns(message_type_add(b, m->type));
    ns(message_instance_status_add(b, m->instance_status));
    switch(m->type){
        case PRE_ACCEPT_OK:
            break;
        default:
            ns(message_data_add(b, instance_data_to_buffer(m, b)));
    }
    if(m->start)ns(message_srt_add(b, m->start));
    if(m->stop)ns(message_stp_add(b, m->stop));
    ns(message_end_as_root(b));
}

