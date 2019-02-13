/**
 * File   : epaxos.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : za 02 feb 2019 20:32
 */

#include "../src/epaxos.c"

struct message *create_message(enum message_type t, char *start_key,
        char *end_key, enum io_t rw ){
    struct command *cmd = malloc(sizeof(struct command));
    strncpy(cmd->span.start_key, start_key, KEY_SIZE);
    strncpy(cmd->span.end_key, end_key, KEY_SIZE);
    memset(cmd->span.max, 0, KEY_SIZE);
    cmd->writing = rw;
    struct message *m  = malloc(sizeof(struct message));
    m->command = cmd;
    m->type = t;
    m->start = 0;
    m->nack = 0;
    m->instance_status = NONE;
    m->ballot = 0;
    for(int ld = 0;ld < MAX_DEPS;ld++){
        m->deps[ld].id.instance_id = 0;
        m->deps[ld].id.replica_id = 0;
        m->deps[ld].committed = 0;
    }
    return m;
}

void copy_command(struct message *m){
    struct command *cmd = malloc(sizeof(struct command));
    *cmd = *m->command;
    m->command = cmd;
}

void check(struct replica *r, enum message_type t, size_t s, size_t rid){
    struct message *m;
    int rc = chan_recv_spsc(&r->sync->chan_io, &m);
    assert(rc > 0);
    assert(m->id.instance_id == 1);
    assert(m->id.replica_id == rid);
    assert(m->type == t);
    assert(m->seq == s);
}

void no_conflict(){
    struct sync s;
    chan_init(&s.chan_io);
    chan_init(&s.chan_eo);
    chan_init(&s.chan_exec);
    struct replica r1;
    struct replica r2;
    new_replica(&r1);
    new_replica(&r2);
    r1.id = 0;
    r2.id = 1;
    r1.sync = &s;
    r2.sync = &s;
    struct message *m = create_message(PHASE1, "10", "20", WRITE);
    assert(propose(&r1, m) == 0);
    check(&r1, PRE_ACCEPT, 1, 0);
    copy_command(m);
    assert(m != 0);
    assert(step(&r2, m) == 0);
    check(&r2, PRE_ACCEPT_OK, 1, 0);
    assert(step(&r1, m) == 0);
    check(&r1, COMMIT, 1, 0);
    destroy_replica(&r1);
    destroy_replica(&r2);
    free(m);
}

void simple_conflict(){
    struct sync s;
    chan_init(&s.chan_io);
    chan_init(&s.chan_eo);
    chan_init(&s.chan_exec);
    struct replica r1;
    struct replica r2;
    new_replica(&r1);
    new_replica(&r2);
    r1.id = 0;
    r2.id = 1;
    r1.sync = &s;
    r2.sync = &s;
    struct message *m1 = create_message(PHASE1, "10", "20", WRITE);
    struct message *m2 = create_message(PHASE1, "06", "18", WRITE);
    assert(propose(&r1, m1) == 0);
    assert(propose(&r2, m2) == 0);
    check(&r1, PRE_ACCEPT, 1, 0);
    check(&r2, PRE_ACCEPT, 1, 1);
    copy_command(m1);
    assert(step(&r2, m1) == 0);
    check(&r2, PRE_ACCEPT_REPLY, 2, 0);
    assert(step(&r1, m1) == 0);
    check(&r1, ACCEPT, 2, 0);
    assert(step(&r2, m1) == 0);
    check(&r2, ACCEPT_REPLY, 2, 0);
    assert(step(&r1, m1) == 0);
    assert(step(&r1, m1) == 0);
    check(&r1, COMMIT, 2, 0);
    destroy_replica(&r1);
    destroy_replica(&r2);
    free(m1);
    free(m2);
}

void recovery(){

}

int main(){
    no_conflict();
    simple_conflict();
    recovery();
    return 0;
}
