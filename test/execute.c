/**
 * File   : execute.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : wo 13 feb 2019 16:22
 */

#include "../src/execute.c"

struct test_scc {
    struct edge *edges;
    int nr_edges;
    int nr_comps;
    int64_t components[5][5];
};

struct edge {
    uint64_t from;
    uint64_t to;
};

struct edge edges0[5] = {
    {2, 1},
    {1, 3},
    {3, 2},
    {1, 4},
    {4, 5}
};

int64_t components0[3][5] = {
    {5, -1, -1, -1, -1},
    {4, -1, -1, -1, -1},
    {1, 2, 3, -1, -1}
};

struct edge edges1[3] = {
    {1, 2},
    {2, 3},
    {3, 4}
};

int64_t components1[4][5] = {
    {4, -1, -1, -1, -1},
    {3, -1, -1, -1, -1},
    {2, -1, -1, -1, -1},
    {1, -1, -1, -1, -1}
};

struct edge edges2[10] = {
    {1, 2},
    {2, 3},
    {3, 1},
    {2, 4},
    {2, 5},
    {2, 7},
    {4, 6},
    {5, 6},
    {7, 5},
    {5, 4}
};

int64_t components2[5][5] = {
    {6, -1, -1, -1, -1},
    {4, -1, -1, -1, -1},
    {5, -1, -1, -1, -1},
    {7, -1, -1, -1, -1},
    {1, 2, 3, -1, -1}
};

struct edge edges3[17] = {
    {1, 2},
    {1, 4},
    {2, 3},
    {2, 5},
    {3, 1},
    {3, 7},
    {4, 3},
    {5, 6},
    {5, 7},
    {6, 7},
    {6, 8},
    {6, 9},
    {6, 10},
    {7, 5},
    {8, 10},
    {9, 10},
    {10, 9}
};

int64_t components3[4][5] = {
    {9, 10, -1, -1, -1},
    {8, -1, -1, -1, -1},
    {5, 6, 7, -1, -1},
    {1, 2, 3, 4, -1}
};

struct edge edges4[6] = {
    {1, 2},
    {2, 3},
    {3, 4},
    {3, 5},
    {4, 1},
    {5, 3}
};

int64_t components4[1][5] = {
    {1, 2, 3, 4, 5}
};


KHASH_MAP_INIT_INT64(nodes, struct instance*)

void insert_edge(khash_t(nodes) *n, uint64_t id){
    khint_t k;
    int absent;
    k = kh_put(nodes, n, id, &absent);
    if(absent){
        struct instance *inst = malloc(sizeof(struct instance));
        memset(inst->deps, 0, sizeof(struct dependency)*MAX_DEPS);
        inst->key.instance_id = id;
        kh_value(n, k) = inst;
    }
}

//insert sort good enough for now
void sort_test_scc(scc *s){
    int i,j;
    for(i = 1;i < MAX_DEPS;i++){
        struct tarjan_node *d = s->nodes[i];
        if(!d) return;
        j = i - 1;
        while(j >= 0 && (s->nodes[j]) && s->nodes[j]->i->key.instance_id >
                d->i->key.instance_id){
            s->nodes[j+1] = s->nodes[j];
            j--;
        }
        s->nodes[j+1] = d;
    }
}

void test_scc(){
    struct test_scc cases[5];
    memset(cases, 0, sizeof(struct test_scc)*5);
    cases[0].edges = edges0;
    cases[0].nr_edges = 5;
    memcpy(cases[0].components, components0, sizeof(int64_t)*15);
    cases[0].nr_comps = 3;
    cases[1].edges = edges1;
    cases[1].nr_edges = 3;
    memcpy(cases[1].components, components1, sizeof(int64_t)*20);
    cases[1].nr_comps = 4;
    cases[2].edges = edges2;
    cases[2].nr_edges = 10;
    memcpy(cases[2].components, components2, sizeof(int64_t)*25);
    cases[2].nr_comps = 5;
    cases[3].edges = edges3;
    cases[3].nr_edges = 17;
    memcpy(cases[3].components, components3, sizeof(int64_t)*20);
    cases[3].nr_comps = 4;
    cases[4].edges = edges4;
    cases[4].nr_edges = 6;
    memcpy(cases[4].components, components4, sizeof(int64_t)*5);
    cases[4].nr_comps = 1;
    khint_t k;
    struct executor *e = new_executor();
    khash_t(nodes) *n = kh_init(nodes);
    int tests = 5;
    for(int i = 0;i < tests;i++){
        kh_clear(nodes, n);
        khint_t k;
        /**
         * DEBUG
            for(k = kh_begin(n);k != kh_end(n);++k){
                if(!kh_exist(n, k)) continue;
                    printf("key: %d\n", k);
                    printf("value: %p\n", kh_value(n, k));
            }
         **/
        for(int j = 0;j < cases[i].nr_edges;j++){
            int absent = 0;
            struct edge edge = cases[i].edges[j];
            insert_edge(n, edge.from);
            insert_edge(n, edge.to);
            k = kh_get(nodes, n, edge.from);
            struct instance *mn = kh_value(n, k);
            for(int l = 0;l < MAX_DEPS;l++){
                if(mn->deps[l].id.instance_id == 0){
                    mn->deps[l].id.instance_id = edge.to;
                    break;
                }
            }
        }
        struct instance *add;
        kh_foreach_value(n, add, {
            add_node(e, add);
        })
        /**
         * DEBUG
        for(k = kh_begin(n);k != kh_end(n);++k){
                if(!kh_exist(n, k)) continue;
                struct instance *add = kh_value(n, k);
                add_node(e, add);
            }
         **/
        strong_connect(e);
        int comp_cnt = cases[i].nr_comps;
        for(int m = 0;m < comp_cnt;m++){
            scc comp = e->sccs[m];
            sort_test_scc(&comp);
            for(int no = 0;no < 5;no++){
                if(cases[i].components[m][no] < 0) continue;
                assert((uint64_t)cases[i].components[m][no] ==
                        comp.nodes[no]->i->key.instance_id);
            }
        }
        reset_exec(e);
        struct instance *del;
        kh_foreach_value(n, del, {
                free(del);
        })
        struct tarjan_node *tn;
        kh_foreach_value(e->vertices, tn, {
                free(tn);
        })
        kh_clear(vertices, e->vertices);
    }
    kh_destroy(vertices, e->vertices);
    free(e);
    kh_destroy(nodes, n);
}

struct test_exec {
    struct instance scc[4];
    uint64_t components[4];
};

struct instance executed[2] = {
    {
        .key = {
            .instance_id = 1,
            .replica_id = 0
        },
        .status = EXECUTED
    },
    {
        .key = {
            .instance_id = 3,
            .replica_id = 0
        },
        .status = EXECUTED
    }
};

struct instance scc0[1] = {
    {
        .key = {
            .instance_id = 4,
            .replica_id = 0
        },
        .status = COMMITTED
    }
};

uint64_t execution0[4] = {4, 0, 0, 0};

struct instance scc1[1] = {
    {
        .key = {
            .instance_id = 4,
            .replica_id = 0
        },
        .status = COMMITTED,
        .deps = {
            {
                .id = {
                    .instance_id = 1,
                    .replica_id = 0
                }
            },
            {
                .id = {
                    .instance_id = 3,
                    .replica_id = 0
                }
            }
        }
    }
};

uint64_t execution1[4] = {4, 0, 0, 0};

struct instance scc2[1] = {
    {
        .key = {
            .instance_id = 4,
            .replica_id = 0
        },
        .status = COMMITTED,
        .deps = {
            {
                .id = {
                    .instance_id = 1,
                    .replica_id = 0
                }
            },
            {
                .id = {
                    .instance_id = 2,
                    .replica_id = 0
                }
            },
            {
                .id = {
                    .instance_id = 3,
                    .replica_id = 0
                }
            }
        }
    }
};

uint64_t execution2[4] = {0};

struct instance scc3[4] = {
    {
        .key = {
            .instance_id = 4,
            .replica_id = 0
        },
        .status = COMMITTED,
        .deps = {
            {
                .id = {
                    .instance_id = 9,
                    .replica_id = 0
                }
            }
        }
    },
    {
        .key = {
            .instance_id = 9,
            .replica_id = 0
        },
        .status = COMMITTED,
        .deps = {
            {
                .id = {
                    .instance_id = 5,
                    .replica_id = 0
                }
            }
        }
    },
    {
        .key = {
            .instance_id = 5,
            .replica_id = 0
        },
        .status = COMMITTED,
        .deps = {
            {
                .id = {
                    .instance_id = 8,
                    .replica_id = 0
                }
            }
        }
    },
    {
        .key = {
            .instance_id = 8,
            .replica_id = 0
        },
        .status = COMMITTED,
        .deps = {
            {
                .id = {
                    .instance_id = 4,
                    .replica_id = 0
                }
            }
        }
    }
};

uint64_t execution3[4] = {4, 5, 8, 9};

struct instance scc4[4] = {
    {
        .key = {
            .instance_id = 4,
            .replica_id = 0
        },
        .status = COMMITTED,
        .deps = {
            {
                .id = {
                    .instance_id = 9,
                    .replica_id = 0
                }
            }
        }
    },
    {
        .key = {
            .instance_id = 9,
            .replica_id = 0
        },
        .status = COMMITTED,
        .deps = {
            {
                .id = {
                    .instance_id = 1,
                    .replica_id = 0
                }
            },
            {
                .id = {
                    .instance_id = 5,
                    .replica_id = 0
                }
            }
        }
    },
    {
        .key = {
            .instance_id = 5,
            .replica_id = 0
        },
        .status = COMMITTED,
        .deps = {
            {
                .id = {
                    .instance_id = 3,
                    .replica_id = 0
                }
            },
            {
                .id = {
                    .instance_id = 8,
                    .replica_id = 0
                }
            }
        }
    },
    {
        .key = {
            .instance_id = 8,
            .replica_id = 0
        },
        .status = COMMITTED,
        .deps = {
            {
                .id = {
                    .instance_id = 1,
                    .replica_id = 0
                }
            },
            {
                .id = {
                    .instance_id = 4,
                    .replica_id = 0
                }
            }
        }
    }
};

uint64_t execution4[4] = {4, 5, 8, 9};

struct instance scc5[4] = {
    {
        .key = {
            .instance_id = 4,
            .replica_id = 0
        },
        .status = COMMITTED,
        .deps = {
            {
                .id = {
                    .instance_id = 2,
                    .replica_id = 0
                }
            },
            {
                .id = {
                    .instance_id = 9,
                    .replica_id = 0
                }
            }
        }
    },
    {
        .key = {
            .instance_id = 9,
            .replica_id = 0
        },
        .status = COMMITTED,
        .deps = {
            {
                .id = {
                    .instance_id = 1,
                    .replica_id = 0
                }
            },
            {
                .id = {
                    .instance_id = 5,
                    .replica_id = 0
                }
            }
        }
    },
    {
        .key = {
            .instance_id = 5,
            .replica_id = 0
        },
        .status = COMMITTED,
        .deps = {
            {
                .id = {
                    .instance_id = 3,
                    .replica_id = 0
                }
            },
            {
                .id = {
                    .instance_id = 8,
                    .replica_id = 0
                }
            }
        }
    },
    {
        .key = {
            .instance_id = 8,
            .replica_id = 0
        },
        .status = COMMITTED,
        .deps = {
            {
                .id = {
                    .instance_id = 1,
                    .replica_id = 0
                }
            },
            {
                .id = {
                    .instance_id = 4,
                    .replica_id = 0
                }
            }
        }
    }
};

//insert sort good enough for now
void sort_executed(struct instance *inst[MAX_DEPS]){
    int i,j;
    struct instance *d = 0;
    for(i = 1;i < MAX_DEPS;i++){
        d = inst[i];
        if(!d) return;
        j = i - 1;
        while(j >= 0 && (inst[j]) && inst[j]->key.instance_id >
                d->key.instance_id){
            inst[j+1] = inst[j];
            j--;
        }
        inst[j+1] = d;
    }
}

uint64_t execution5[4] = {0};

void test_execution(){
    struct test_exec cases[6];
    memset(cases, 0, sizeof(struct test_exec)*6);
    memcpy(cases[0].scc, scc0, 1*sizeof(struct instance));
    memcpy(cases[0].components, execution0, 4*sizeof(uint64_t));
    memcpy(cases[1].scc, scc1, 1*sizeof(struct instance));
    memcpy(cases[1].components, execution1, 4*sizeof(uint64_t));
    memcpy(cases[2].scc, scc2, 1*sizeof(struct instance));
    memcpy(cases[2].components, execution2, 4*sizeof(uint64_t));
    memcpy(cases[3].scc, scc3, 4*sizeof(struct instance));
    memcpy(cases[3].components, execution3, 4*sizeof(uint64_t));
    memcpy(cases[4].scc, scc4, 4*sizeof(struct instance));
    memcpy(cases[4].components, execution4, 4*sizeof(uint64_t));
    memcpy(cases[5].scc, scc5, 4*sizeof(struct instance));
    memcpy(cases[5].components, execution5, 4*sizeof(uint64_t));
    struct replica r;
    assert(new_replica(&r) == 0);
    for(int i = 0;i < 2;i++){
        struct instance *ni = malloc(sizeof(struct instance));
        assert((ni));
        *ni = executed[i];
        assert(register_instance(&r, ni) == 0);
    }
    struct executor *exec;
    assert((exec = new_executor()));
    exec->r = &r;
    int tests = 6;
    int c;
    int ic;
    int cc;
    for(c = 0;c < tests;c++){
        struct instance *is = cases[c].scc;
        for(ic = 0;ic < 4;ic++){
            if(is[ic].key.instance_id == 0) continue;
            add_node(exec, &is[ic]);
        }
        strong_connect(exec);
        assert(exec->scc_count == 1);
        scc *comp = &exec->sccs[0];
        execute_scc(exec, comp);
        sort_executed(exec->executed);
        uint64_t executed[4] = {0};
        int excd = 0;
        for(int x = 0;x < 4;x++){
            struct instance *execd = exec->executed[x];
            if((!execd) || !is_sstate(execd->status, EXECUTED)) continue;
            executed[excd] = execd->key.instance_id;
            excd++;
        }
        for(cc = 0;cc < 4;cc++){
            uint64_t comps = cases[c].components[cc];
            uint64_t scc = executed[cc];
            if(comps != scc){
                uint64_t sccc = scc;
            }
            assert(cases[c].components[cc] == executed[cc]);
        }
        reset_exec(exec);
    }
    destroy_replica(&r);
    struct tarjan_node *tn;
    kh_foreach_value(exec->vertices, tn, {
            free(tn);
    })
    kh_destroy(vertices, exec->vertices);
    free(exec);
}

int main(){
    test_scc();
    test_execution();
    return 0;
}
