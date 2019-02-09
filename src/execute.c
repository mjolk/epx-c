/**
 * File   : src/execute.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : Sat 13 Oct 2018 15:35
 */

#include <errno.h>
#include "node.h"

typedef struct scc {
    struct tarjan_node *nodes[MAX_DEPS];
} scc;

struct tarjan_node {
    struct instance *i;
    int index;
    int low_link;
    int on_stack;
    struct tarjan_node *deps[MAX_DEPS];
    SLL_ENTRY(tarjan_node) next;
};

int contains(scc *comp, struct tarjan_node *t){
    for(int i = 0;i < MAX_DEPS;i++){
        if(comp->nodes[i] == t) return 1;
    }
    return 0;
}

struct tarjan_node *new_tn(struct instance *i){
    struct tarjan_node *node = malloc(sizeof(struct tarjan_node));
    if(!node){ errno = ENOMEM; return 0;}
    node->i = i;
    node->index = -1;
    node->low_link = -1;
    node->on_stack = 0;
    memset(node->deps, 0, sizeof(struct tarjan_node*)*MAX_DEPS);
    return node;
}

int not_visited(struct tarjan_node *n){
    return (n->index < 0);
}

void reset_node(struct tarjan_node *n){
    n->index = -1;
    n->low_link = -1;
    n->on_stack = 0;
    memset(n->deps, 0, sizeof(struct tarjan_node)*MAX_DEPS);
}

int min(int n1, int n2){
    return (n1 < n2)?n1:n2;
}

SLL_HEAD(stack, tarjan_node);
KHASH_MAP_INIT_INT64(vertices, struct tarjan_node*);

struct executor {
    khash_t(vertices) *vertices;
    int scc_count;
    struct stack stack;
    scc sccs[MAX_DEPS];
    int index;
};

void reset_exec(struct executor *e){
    e->scc_count = 0;
    memset(e->sccs, 0, sizeof(scc)*MAX_DEPS);
    SLL_INIT(&e->stack);
    e->index = 0;
}

struct executor *new_executor(){
    struct executor *e = malloc(sizeof(struct executor));
    if(!e){ errno = ENOMEM; return 0;}
    e->vertices = kh_init(vertices);
    e->scc_count = 0;
    SLL_INIT(&e->stack);
    memset(e->sccs, 0, sizeof(scc)*MAX_DEPS);
    e->index = 0;
    return e;
}

struct tarjan_node *pop(struct executor *e){
    struct tarjan_node *ret = SLL_FIRST(&e->stack);
    if(ret){
        SLL_REMOVE_HEAD(&e->stack, next);
    }
    return ret;
}

void push(struct executor *e, struct tarjan_node *tn){
    SLL_INSERT_HEAD(&e->stack, tn, next);
}

void visit(struct executor *e, struct tarjan_node *n){
    n->index = e->index;
    n->low_link = e->index;
    e->index++;
    n->on_stack = 1;
    push(e, n);
    struct tarjan_node *tn;

    for(int i = 0;i < MAX_DEPS;i++){
        if((tn = n->deps[i])){
            if(not_visited(tn)){
                visit(e, tn);
                n->low_link = min(n->low_link, tn->low_link);
            } else if(tn->on_stack){
                n->low_link = min(n->low_link, tn->index);
            }
        }
    }

    tn = 0;
    if(n->low_link == n->index){
        int cnt = 0;
        while((tn = pop(e)) && (tn != n)){
            tn->on_stack = 0;
            e->sccs[e->scc_count].nodes[cnt] = tn;
            cnt++;
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
            for(int dc = 0;dc < MAX_DEPS;dc++){
                if((k = kh_get(vertices, e->vertices, dh_key(
                                    &tn->i->deps[dc].id))) &&
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

int add_node(struct executor *e, struct instance *i){
    int added = 0;
    struct tarjan_node *nn = new_tn(i);
    if(nn){
        khint_t key = kh_put(vertices, e->vertices, dh_key(&i->key), &added);
        kh_value(e->vertices, key) = nn;
        added = 1;
    }
    return added;
}

//insert sort good enough for now
void sort_scc(scc *s){
    int j = 0;
    for(int i = 1;i < MAX_DEPS;i++){
        struct tarjan_node *d = s->nodes[i];
        j = i - 1;
        while(j >= 0 && s->nodes[j]->i->seq > d->i->seq ){
            s->nodes[j +1] = s->nodes[j];
            j--;
        }
        s->nodes[j+1] = d;
    }
}

void execute_scc(struct node *n, scc *comp){
    struct executor *e = n->exec;
    for(int j = 0;j < MAX_DEPS;j++){
        struct tarjan_node *v = comp->nodes[j];
        for(int k = 0;k < MAX_DEPS;k++){
            struct dependency dep = v->i->deps[k];
            khint_t k;
            if((k = kh_get(vertices, e->vertices, dh_key(&dep.id))) &&
                    k != kh_end(e->vertices) &&
                    contains(comp, kh_value(e->vertices, k)) ){
                continue;

            }
            struct instance *i = find_instance(&n->r, &dep.id);
            if(!is_state(i->status, EXECUTED)){
                return;
            }
        }
    }
    sort_scc(comp);
    for(int i = 0;i < MAX_DEPS;i++){

    }

}

void execute(struct node *n){
    strong_connect(n->exec);
    for(int i = 0;i < n->exec->scc_count;i++){
        execute_scc(n, &n->exec->sccs[i]);
    }
    reset_exec(n->exec);
}


void read_committed(struct node *n, int rep){
    struct instance *i;
    while(n->running){
        i = chan_recv(&n->chan_exec);
        if(i){
            if(add_node(n->exec, i)){
                execute(n);
            }
        }
    }
}
