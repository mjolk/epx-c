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

coroutine void check1(int ch, int done, enum message_type t, size_t s){
    struct message *m;
    int rc = chrecv(ch, &m, MSG_SIZE, -1);
    assert(rc >= 0);
    assert(m->id.instance_id == 1);
    assert(m->id.replica_id == 0);
    assert(m->type == t);
    assert(m->seq == s);
    chsend(done, &m, MSG_SIZE, -1);
}

void no_conflict(){
    struct replica r1;
    struct replica r2;
    struct message *m = create_message(PHASE1, "10", "20", WRITE);
    int done_ch[2];
    assert(chmake(done_ch) >= 0);
    int b = bundle();
    r1.id = 0;
    r2.id = 1;
    assert(new_replica(&r1) == 0);
    assert(new_replica(&r2) == 0);
    assert(bundle_go(b, check1(r1.chan_io[0], done_ch[0], PRE_ACCEPT, 1)) >= 0);
    assert(propose(&r1, m) == 0);
    assert(chrecv(done_ch[1], &m, MSG_SIZE, -1) >= 0);
    assert(bundle_go(b, check1(r2.chan_io[0], done_ch[0], PRE_ACCEPT_OK, 1)) >= 0);
    copy_command(m);
    assert(m != 0);
    assert(step(&r2, m) == 0);
    assert(chrecv(done_ch[1], &m, MSG_SIZE, -1) >= 0);
    assert(bundle_go(b, check1(r1.chan_exec[0], done_ch[0], COMMIT, 1)) >= 0);
    assert(step(&r1, m) == 0);
    assert(chrecv(done_ch[1], &m, MSG_SIZE, -1) >= 0);
    destroy_replica(&r1);
    destroy_replica(&r2);
    hclose(done_ch[0]);
    hclose(done_ch[1]);
    hclose(b);
    free(m);
}

void simple_conflict(){
    struct replica r1;
    struct replica r2;
    struct message *m1 = create_message(PHASE1, "10", "20", WRITE);
    struct message *m2 = create_message(PHASE1, "06", "18", WRITE);
    int done_ch[2];
    assert(chmake(done_ch) >= 0);
    int b = bundle();
    r1.id = 0;
    r2.id = 1;
    assert(new_replica(&r1) == 0);
    assert(new_replica(&r2) == 0);
    assert(bundle_go(b, check1(r1.chan_io[0], done_ch[0], PRE_ACCEPT, 1)) >= 0);
    assert(propose(&r1, m1) == 0);
    assert(propose(&r2, m2) == 0);
    assert(chrecv(done_ch[1], &m1, MSG_SIZE, -1) >= 0);
    assert(bundle_go(b, check1(r2.chan_io[0], done_ch[0], PRE_ACCEPT_REPLY, 2)) >= 0);
    copy_command(m1);
    assert(step(&r2, m1) == 0);
    assert(chrecv(done_ch[1], &m1, MSG_SIZE, -1) >= 0);
    assert(bundle_go(b, check1(r1.chan_io[0], done_ch[0], ACCEPT, 2)) >= 0);
    assert(step(&r1, m1) == 0);
    assert(chrecv(done_ch[1], &m1, MSG_SIZE, -1) >= 0);
    assert(bundle_go(b, check1(r2.chan_io[0], done_ch[0], ACCEPT_REPLY, 2)) >= 0);
    assert(step(&r2, m1) == 0);
    assert(chrecv(done_ch[1], &m1, MSG_SIZE, -1) >= 0);
    assert(bundle_go(b, check1(r1.chan_io[0], done_ch[0], COMMIT, 2)) >= 0);
    assert(step(&r1, m1) == 0);
    assert(step(&r1, m1) == 0);
    assert(chrecv(done_ch[1], &m1, MSG_SIZE, -1) >= 0);
    destroy_replica(&r1);
    destroy_replica(&r2);
    hclose(done_ch[0]);
    hclose(done_ch[1]);
    hclose(b);
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
