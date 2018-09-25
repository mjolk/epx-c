/**
 * File   : src/message.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : di 11 sep 2018 23:26
 */
#include "message.h"


void instance_data_from_buffer(struct message *m, const void *buffer) {
    command_from_buffer(m->command, buffer);
    m->seq = ns(instance_data_seq(buffer));
    ns(dependency_vec_t) rdps = ns(instance_data_deps(buffer));
    size_t veclen = ns(dependency_vec_len(rdps));
    size_t len = (veclen > N)?N:veclen;
    for(size_t i = 0; i < len; i++) {
        ns(dependency_struct_t) sid = ns(dependency_vec_at(rdps, i));
        struct instance_id ni = {.replica_id = ns(dependency_replica_id(sid)),
            .instance_id = ns(dependency_instance_id(sid))};
        struct dependency *dep = new_dependency();
        dep->id = ni;
        dep->committed = ns(dependency_committed(sid));
        m->deps[i] = dep;
    }
}

int message_from_buffer(struct message *m, void *buffer) {
    ns(message_table_t) bm = ns(message_as_root(buffer));
    if(!bm) goto error;
    m->to = ns(message_to(bm));
    m->ballot = ns(message_ballot(bm));
    ns(instance_id_struct_t) i_id = ns(message_instance_id(bm));
    if(!i_id) goto error;
    m->id.replica_id = ns(instance_id_replica_id(i_id));
    m->id.instance_id = ns(instance_id_instance_id(i_id));
    if(!ns(message_type_is_present(bm))) {
        goto error;
    }
    m->type = ns(message_type(bm));
    switch(m->type){
        case NACK:
        case PRE_ACCEPT_OK:
            break;
        default:
            instance_data_from_buffer(m, ns(message_data(bm)));
    }
    return 1;
error:
    return -1;
}

void instance_data_to_buffer(struct message *m, flatcc_builder_t *b) {
    ns(instance_data_start(b));
    if(m->command){
        command_to_buffer(m->command, b);
    }
    ns(instance_data_seq_add(b, m->seq));
    ns(instance_data_deps_start(b));
    for(size_t i = 0; i < N; i++){
        ns(instance_data_deps_push_create(b,
                    m->deps[i]->id.replica_id, m->deps[i]->id.instance_id,
                    m->deps[i]->committed));
    }
    ns(instance_data_deps_end(b));
    ns(instance_data_end(b));
}

void message_to_buffer(struct message *m, flatcc_builder_t *b) {
    size_t s;
    ns(message_start_as_root(b));
    ns(message_to_add(b, m->to));
    ns(message_ballot_add(b, m->ballot));
    ns(message_instance_id_create(b, m->id.replica_id, m->id.instance_id));
    ns(message_type_add(b, m->type));
    switch(m->type){
        case NACK:
        case PRE_ACCEPT_OK:
            break;
        default:
            instance_data_to_buffer(m, b);
    }
    ns(message_end_as_root(b));
}

