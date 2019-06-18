/**
 * File   : epaxos.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 03:39
 */
#include "rocksdb/c.h"
#ifdef __cplusplus
extern "C" {
#endif
#include <unistd.h>
#include "../include/epx.h"
#include <string.h>
#include "storage.h"
#include <stdio.h>

struct store {
    rocksdb_t *db;
    rocksdb_options_t *options;
    rocksdb_readoptions_t *r_options;
    rocksdb_writeoptions_t *w_options;
    rocksdb_writebatch_t *w_batch;
    char *error;
};

int open_store(struct store *store){
    store->options = rocksdb_options_create();
    long cpus = sysconf(_SC_NPROCESSORS_ONLN);  // get # of online cores
    rocksdb_options_increase_parallelism(store->options, (int)(cpus));
    rocksdb_options_optimize_level_style_compaction(store->options, 0);
    // create the DB if it's not already present
    rocksdb_options_set_create_if_missing(store->options, 1);
    store->db = rocksdb_open(store->options, DB_PATH, &store->error);
    if(store->error) { printf("error: %s\n", store->error); return -1;}
    store->r_options = rocksdb_readoptions_create();
    store->w_options = rocksdb_writeoptions_create();
    store->w_batch = rocksdb_writebatch_create();
    return 0;
}

void close_store(struct store *store){
    rocksdb_writeoptions_destroy(store->w_options);
    rocksdb_readoptions_destroy(store->r_options);
    rocksdb_options_destroy(store->options);
    rocksdb_writebatch_destroy(store->w_batch);
    rocksdb_close(store->db);
}

char* store_get(struct store *store, struct kv *kv, size_t *l){
    return rocksdb_get(store->db, store->r_options, kv->key,
        KEY_SIZE -1, l, &store->error);
}

void store_put(struct store *store, struct kv *kv){
    rocksdb_put(store->db, store->w_options, kv->key, KEY_SIZE -1, kv->value,
        strlen(kv->value) + 1, &store->error);
}

void store_batch_put(struct store *store, struct kv **kvs, int len){
    if(!store->w_batch) return;
    rocksdb_writebatch_clear(store->w_batch);
    for(int i = 0;i < len;i++){
        rocksdb_writebatch_put(store->w_batch, kvs[i]->key, KEY_SIZE -1,
            kvs[i]->value, strlen(kvs[i]->value) + 1);
    }
    rocksdb_write(store->db, store->w_options, store->w_batch, &store->error);
}
#ifdef __cplusplus
}  /* end extern "C" */
#endif