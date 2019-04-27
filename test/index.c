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
    struct span s[1];
    strcpy(s[0].start_key, start_key);
    strcpy(s[0].end_key, end_key);
    memset(s[0].max, 0, KEY_SIZE);
    struct command *cmd = malloc(sizeof(struct command));
    assert(cmd != 0);
    memcpy(cmd->spans, s, sizeof(struct span));
    cmd->id = 1111;
    cmd->writing = rw;
    cmd->value = 0;
    cmd->tx_size = 1;
    struct instance *i = malloc(sizeof(struct instance));
    assert(i != 0);
    i->key.replica_id = rid;
    i->command = cmd;
    i->ballot = 1;
    i->lt = 0;
    i->status = ACCEPTED;
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
    struct instance *inst;
    LLRB_INIT(&index);
    for(int c = 0;c < 8;c++){
        struct instance *i = create_instance(2, setup[c][0], setup[c][1], WRITE);
        i->key.instance_id = c;
        i->key.replica_id = 1;
        if(c == 7){
            i->status = COMMITTED;
            inst = i;
        }
        LLRB_INSERT(instance_index, &index, i);
    }
    struct command cmd = {
        .id = 1,
        .writing = WRITE,
        .tx_size = 1
    };
    struct span ns = {
        .start_key = "02",
        .end_key = "09"
    };
    memset(ns.max, 0, KEY_SIZE);
    memcpy(cmd.spans, &ns, sizeof(struct span));
    struct seq_deps_probe p = {
        .updated = 0
    };
    uint64_t seq = seq_deps_for_command(&index, &cmd, 0, &p);
    assert(p.updated > 0);
    assert(p.deps[0].id.instance_id == 7);
    assert(p.deps[1].id.instance_id == 3);
    p.deps[0].committed = 0;
    strcpy(ns.start_key, "01");
    strcpy(ns.end_key, "03");
    memset(ns.max, 0, KEY_SIZE);
    memcpy(cmd.spans, &ns, sizeof(struct span));
    p.deps[0].id.instance_id = 9;
    p.deps[1].id.replica_id = 1;
    struct instance *pac_inst = pre_accept_conflict(&index, inst, &cmd, 0,
      p.deps);
    assert(pac_inst->key.instance_id == 0);
    return 0;
}

