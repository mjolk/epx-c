/**
 * File   : src/execute.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : Sat 13 Oct 2018 15:35
 */

#include <errno.h>
#include <sys/queue.h>
#include "execute.h"
#include "storage.h"

typedef struct scc {
    struct tarjan_node *nodes[MAX_DEPS];
} scc;

struct tarjan_node {
    struct instance *i;
    int index;
    int low_link;
    int on_stack;
    struct tarjan_node *deps[MAX_DEPS];
    SLIST_ENTRY(tarjan_node) next;
};

int contains(scc *comp, struct tarjan_node *t){
    for(int i = 0;i < MAX_DEPS;i++){
        if(comp->nodes[i] == t) return 1;
    }
    return 0;
}

struct tarjan_node *new_tn(struct instance *i){
    struct tarjan_node *node = (struct tarjan_node*)malloc(
        sizeof(struct tarjan_node));
    if(!node){ errno = ENOMEM; return 0;}
    node->i = i;
    node->index = -1;
    node->low_link = -1;
    node->on_stack = 0;
    node->next.sle_next = 0;
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
    n->next.sle_next = 0;
    memset(n->deps, 0, sizeof(struct tarjan_node*)*MAX_DEPS);
}

int min(int n1, int n2){
    return (n1 < n2)?n1:n2;
}

SLIST_HEAD(stack, tarjan_node);
KHASH_MAP_INIT_INT64(vertices, struct tarjan_node*);

struct executor {
    struct io_sync *out;
    khash_t(vertices) *vertices;
    int scc_count;
    struct stack stack;
    scc sccs[MAX_DEPS];
    struct instance *executed[MAX_DEPS];
    int index;
};

void reset_exec(struct executor *e){
    e->scc_count = 0;
    memset(e->sccs, 0, sizeof(scc)*MAX_DEPS);
    memset(e->executed, 0, sizeof(struct instance*)*MAX_DEPS);
    SLIST_INIT(&e->stack);
    e->index = 0;
}

void new_executor(struct executor *e){
    e->vertices = kh_init(vertices);
    SLIST_INIT(&e->stack);
}

struct tarjan_node *pop(struct executor *e){
    struct tarjan_node *ret = SLIST_FIRST(&e->stack);
    if(ret) SLIST_REMOVE_HEAD(&e->stack, next);
    return ret;
}

void push(struct executor *e, struct tarjan_node *tn){
    SLIST_INSERT_HEAD(&e->stack, tn, next);
}

void visit(struct executor *e, struct tarjan_node *n){
    n->index = e->index;
    n->low_link = e->index;
    e->index++;
    n->on_stack = 1;
    push(e, n);
    struct tarjan_node *tn = 0;

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
        while(tn != n){
            tn = pop(e);
            tn->on_stack = 0;
            e->sccs[e->scc_count].nodes[cnt] = tn;
            cnt++;
        }
        e->scc_count++;
    }

}

void strong_connect(struct executor *e){
    khint_t k, l;
    struct tarjan_node *tn;
    kh_foreach_value(e->vertices, tn, {
        reset_node(tn);
        for(int dc = 0;dc < MAX_DEPS;dc++){
            if(tn->i->deps[dc].id.instance_id == 0) continue;
            l = kh_get(vertices, e->vertices, dh_key(&tn->i->deps[dc].id));
            if(l == kh_end(e->vertices)) continue;
            tn->deps[dc] = kh_value(e->vertices, l);
        }
    })

    kh_foreach_value(e->vertices, tn, {
        if(not_visited(tn)){
            visit(e, tn);
        }
    })
}

int add_node(struct executor *e, struct instance *i){
    int added = 0;
    khint_t k;
    struct tarjan_node *nn = new_tn(i);
    if(nn){
        k = kh_put(vertices, e->vertices, dh_key(&i->key), &added);
        if(!added && kh_exist(e->vertices, k)){
            free(kh_value(e->vertices, k));
        }
        kh_value(e->vertices, k) = nn;
    }
    return added;
}

//insert sort good enough for now
void sort_scc(scc *s){
    int i,j;
    for(i = 1;i < MAX_DEPS;i++){
        struct tarjan_node *d = s->nodes[i];
        if(!d) continue;
        j = i - 1;
        while(j >= 0 && (s->nodes[j]) && s->nodes[j]->i->seq > d->i->seq ){
            s->nodes[j+1] = s->nodes[j];
            j--;
        }
        s->nodes[j+1] = d;
    }
}

void execute_scc(struct executor *e, scc *comp){
    khint_t k;
    int i, j;
    for(i = 0;i < MAX_DEPS;i++){
        struct tarjan_node *v = comp->nodes[i];
        if(!v) continue;
        for(j = 0;j < MAX_DEPS;j++){
            struct dependency dep = v->i->deps[j];
            if(dep.id.instance_id == 0) continue;
            k = kh_get(vertices, e->vertices, dh_key(&dep.id));
            if((k != kh_end(e->vertices)) &&
                    contains(comp, kh_value(e->vertices, k))){
                continue;

            }
            //another thread will never set status to executed
            //so we don't have to synchronize to get a reliable read?
            struct instance *i = find_instance(v->i->r, &dep.id);
            if((!i) || (!is_state(i->status, EXECUTED))) return;
        }
    }
    sort_scc(comp);
    for(i = 0;i < MAX_DEPS;i++){
        if(!comp->nodes[i]) continue;
        struct instance *executed = comp->nodes[i]->i;
        e->executed[i] = executed;//keep for testing
        k = kh_get(vertices, e->vertices, dh_key(&executed->key));
        if(k != kh_end(e->vertices)) kh_del(vertices, e->vertices, k);
        free(comp->nodes[i]);
        executed->status = EXECUTED;
    }
}

void execute(struct executor *e){
    strong_connect(e);
    for(int i = 0;i < e->scc_count;i++){
        execute_scc(e, &e->sccs[i]);
    }
    reset_exec(e);
}

void* run_execute(void* nio){
    struct node_io *io = (struct node_io*)nio;
    struct executor e;
    reset_exec(&e);
    new_executor(&e);
    e.out = &io->io_sync;
    struct instance *i = 0;
    while(io->running){
        if(!chan_recv_spsc(&e.out->chan_exec, &i)){
            continue;
        }
        if(i){
            if(add_node(&e, i)){
                execute(&e);
            }
        }
    }
    pthread_exit(NULL);
    return 0;
}
