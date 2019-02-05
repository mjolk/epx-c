/**
 * File   : instance.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : ma 28 jan 2019 18:03
 */

#include "../src/instance.h"

struct dependency tdeps[MAX_DEPS];
struct dependency tdeps2[MAX_DEPS];

struct instance *create_instance(size_t rid, const char *start_key,
        const char *end_key, enum io_t rw){
    struct span s;
    strncpy(s.start_key, start_key, KEY_SIZE);
    strncpy(s.end_key, end_key, KEY_SIZE);
    memset(s.max, 0, KEY_SIZE);
    struct command *cmd = malloc(sizeof(struct command));
    assert(cmd != 0);
    cmd->span = s;
    cmd->id = 1111;
    cmd->writing = rw;
    memset(cmd->value, 0, VALUE_SIZE);
    struct instance *i = malloc(sizeof(struct instance));
    assert(i != 0);
    i->key.replica_id = rid;
    i->command = cmd;
    i->ballot = 1;
    i->lt = 0;
    i->status = PRE_ACCEPTED;
    return i;
}

void free_instance(struct instance *i){
    if(i->command != 0){
        free(i->command);
    }
    if(i->lt != 0){
        free(i->lt);
    }
    free(i);
}

void new_deps(){
    for(int i = 0;i < MAX_DEPS;i++){
        tdeps[i].id.instance_id = 1000+i;
        tdeps[i].id.replica_id = i;
        tdeps[i].committed = (((i+2)%2) > 0)?1:0;
        tdeps2[i].id.instance_id = 1000+i;
        tdeps2[i].id.replica_id = i;
        tdeps2[i].committed = (((i+2)%2) > 0)?1:0;
    }
}

int main(){
    new_deps();
    assert(equal_deps(tdeps, tdeps2) == 1);
    tdeps2[0].id.instance_id = 500;
    assert(equal_deps(tdeps, tdeps2) == 0);
    assert(is_state(PRE_ACCEPTED, (PRE_ACCEPTED | PRE_ACCEPTED_EQ)) == 1);
    assert(is_state(PREPARING, (PRE_ACCEPTED | PRE_ACCEPTED_EQ)) == 0);
    tdeps[MAX_DEPS-1].id.instance_id = 0;
    struct instance_id nid = {
        .instance_id = 1009,
        .replica_id = 1
    };
    struct dependency ndep = {
       .id = nid,
       .committed = 0
    };
    assert(update_deps(tdeps, &ndep) > 0);
    assert(tdeps[MAX_DEPS-1].id.instance_id == 1009);
    assert(tdeps[MAX_DEPS-1].committed == 0);
    assert(update_deps(tdeps, &ndep) == 0);
    struct instance *i = create_instance(1, "10", "20", WRITE);
    assert(i != 0);
    memcpy(i->deps, tdeps, DEPSIZE);
    i->seq = 2;
    struct message *m = message_from_instance(i);
    assert(m != 0);
    assert(m->ballot == 1);
    assert(m->command->id == 1111);
    assert(m->command->writing == WRITE);
    assert(strncmp(m->command->span.start_key, "10", KEY_SIZE) == 0);
    assert(strncmp(m->command->span.end_key, "20", KEY_SIZE) == 0);
    assert(m->id.replica_id == 1);
    assert(m->seq == 2);
    for(int l = 0;l < MAX_DEPS;l++){
        assert(eq_instance_id(&i->deps[l].id, &m->deps[l].id));
        assert(i->deps[l].committed == m->deps[l].committed);
    }
    struct instance_id mid = {
        .instance_id = 345,
        .replica_id = 4
    };
    struct message msg;
    msg.id = mid;
    msg.command = i->command;
    msg.seq = 6;
    memcpy(msg.deps, tdeps2, DEPSIZE);
    struct instance *i2 = instance_from_message(&msg);
    assert(i2 != 0);
    assert(i2->command->id == 1111);
    assert(i2->command->writing == WRITE);
    assert(strncmp(i2->command->span.start_key, "10", KEY_SIZE) == 0);
    assert(strncmp(i2->command->span.end_key, "20", KEY_SIZE) == 0);
    assert(i2->key.instance_id == 345);
    assert(i2->key.replica_id == 4);
    assert(i2->seq == 6);
    uint8_t ub = unique_ballot(0, 1);
    assert(replica_from_ballot(ub) == 1);
    assert(is_initial_ballot(ub) == 1);
    uint8_t ubl = larger_ballot(ub, 1);
    assert(ubl > ub);
    assert(is_initial_ballot(ubl) == 0);
    assert(has_uncommitted_deps(i) == 1);
    i->status = EXECUTED;
    assert(is_committed(i) == 1);
    assert(is_committed(i2) == 0);
    free_instance(i);
    free(i2);
    free(m);
    return 0;
}
