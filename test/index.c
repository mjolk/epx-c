/**
 * File   : index.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : wo 30 jan 2019 05:16
 */

#include <inttypes.h>
#include <stdio.h>
#include "../src/index.h"

void print_tree(struct span *n)
{
    struct span *left, *right;

    if (n == NULL) {
        printf("nil");
        return;
    }
    left = LLRB_LEFT(n, entry);
    right = LLRB_RIGHT(n, entry);
    if (left == NULL && right == NULL)
        printf("END [range %s, %s max: %s]", n->start_key, n->end_key, n->max);
    else {
        printf("LR[range %s, %s max: %s](", n->start_key, n->end_key, n->max);
        print_tree(left);
        printf(",");
        print_tree(right);
        printf(")\n");
    }
}

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

char *setup[8][2] = {
    {"01", "02"},
    {"05", "10"},
    {"16", "32"},
    {"02", "08"},
    {"50", "60"},
    {"40", "55"},
    {"23", "42"},
    {"08", "43"}
};

int main(){
    struct instance_index index;
    LLRB_INIT(&index);
    for(int c = 0;c < 8;c++){
        struct instance *i = create_instance(2, setup[c][0], setup[c][1], WRITE);
        i->key.instance_id = c;
        LLRB_INSERT(instance_index, &index, i);
    }
    struct instance *inst;
    LLRB_FOREACH(inst, instance_index, &index){
        printf("instance id: %"PRIu64" start_key: %s end_key: %s\n",
                inst->key.instance_id, inst->command->span.start_key,
                inst->command->span.end_key);
    }
    struct command cmd = {
        .id = 1,
        .writing = WRITE
    };
    strncpy(cmd.span.start_key, "02", KEY_SIZE);
    strncpy(cmd.span.end_key, "09", KEY_SIZE);
    strncpy(cmd.span.max, "00", KEY_SIZE);
    struct seq_deps_probe p = {
        .updated = 0
    };
    uint64_t seq = seq_deps_for_command(&index, &cmd, 0, &p);
    printf("next sequence nr: %" PRIu64 "dependencies updated: %d\n",
            seq, p.updated);
    for(int i = 0;i < MAX_DEPS;i++){
        printf("dep instance_id: %" PRIu64 "\n", p.deps[i].id.instance_id);
    }
    return 0;
}

