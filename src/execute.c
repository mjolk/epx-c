/**
 * File   : src/execute.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : Sat 13 Oct 2018 15:35
 */
#include "node.h"

typedef struct scc {
    struct tarjan_node *nodes[64];
} scc;

struct tarjan_node {
    struct instance *i;
    int index;
    int low_link;
    int on_stack;
    struct tarjan_node *deps[N];
    SLL_ENTRY(tarjan_node) next;
};

int not_visited(struct tarjan_node *n){
    return (n->index < 0);
}

void reset_node(struct tarjan_node *n){
    n->index = -1;
    n->low_link = -1;
    n->on_stack = 0;
    for(int i = 0;i < N;i++){
        n->deps[i] = 0;
    }
}

int min(int n1, int n2){
    return (n1 < n2)?n1:n2;
}

SLL_HEAD(tn_list, tarjan_node);
KHASH_MAP_INIT_INT64(vertices, struct tarjan_node*);

struct executor {
    khash_t(vertices) *vertices;
    size_t stack_size;
    int scc_count;
    struct tn_list stack;
    scc sccs[256];
    int index;
};

struct tarjan_node *pop(struct tn_list *s){
    struct tarjan_node *ret = SLL_FIRST(s);
    if(ret) SLL_REMOVE_HEAD(s, next);
    return ret;
}

void push(struct tn_list *s, struct tarjan_node *tn){
    SLL_INSERT_HEAD(s, tn, next);
}

void visit(struct executor *e, struct tarjan_node *n){
    n->index = e->index;
    n->low_link = e->index;
    e->index++;
    n->on_stack = 1;
    e->stack_size++;
    push(&e->stack, n);

    for(int i = 0;i < N;i++){
        struct tarjan_node *dep = n->deps[i];
        if(dep){
            if(not_visited(dep)){
                visit(e, dep);
                n->low_link = min(n->low_link, dep->low_link);
            } else if(dep->on_stack){
                n->low_link = min(n->low_link, dep->index);
            }
        }
    }

    if(n->low_link == n->index){
        for(size_t sc = 0;sc < e->stack_size;sc++){
            struct tarjan_node *tn = pop(&e->stack);
            if(tn == n) break;
            if(tn){
                tn->on_stack = 0;
                e->sccs[e->scc_count].nodes[sc] = tn;
            }
        }
        e->scc_count++;
    }

}

void strong_connect(struct executor *e){
    khint_t k;
    for(k = kh_begin(e->vertices);k != kh_end(e->vertices);++k){
        if(kh_exist(e->vertices, k)){
            struct tarjan_node *tn = kh_value(e->vertices, k);
            reset_node(tn);
            for(int dc = 0;dc < N;dc++){
                if((k = kh_get(vertices, e->vertices, dh_key(
                                    tn->i->deps[dc].id.instance_id,
                                    tn->i->deps[dc].id.replica_id))) &&
                        k != kh_end(e->vertices)){
                    tn->deps[dc] = kh_value(e->vertices, k);
                }
            }

        }
    }
    for(k = kh_begin(e->vertices);k != kh_end(e->vertices);++k){
        if(kh_exist(e->vertices, k)){
            struct tarjan_node *vtn = kh_value(e->vertices, k);
            if(not_visited(vtn)){
                visit(e, vtn); 
            }
        }
    }
}

struct message *read_exec(struct node *n){
    return recv(&n->chan_exec);
}

coroutine void execute(struct node *n, int rep){
    struct message *m;
    while(n->running){
        m = read_exec(n);
    }
}
