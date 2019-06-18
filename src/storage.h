/**
 * File   : epaxos.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : do 06 sep 2018 03:39
 */
struct store;
struct kv {
    char key[KEY_SIZE];
    char *value;
};

int open_store(struct store*);
char* store_get(struct store*, struct kv*, size_t*);
void store_put(struct store*, struct kv*);
void store_batch_put(struct store*, struct kv**, int);