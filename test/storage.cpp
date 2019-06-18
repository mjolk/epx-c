/**
 * File   : instance.c
 * License: MIT/X11
 * Author : Dries Pauwels <2mjolk@gmail.com>
 * Date   : ma 28 jan 2019 18:03
 */
#include "../src/storage.cpp"
#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <assert.h>

int main(){
    struct store ns;
    ns.error = NULL;
    assert(open_store(&ns) == 0);
    size_t len;
    struct kv *item = (kv*)malloc(sizeof(struct kv));
    assert(item);
    strcpy(item->key, "01");
    item->value = "some value";
    store_put(&ns, item);
    assert(!ns.error);
    char *val = store_get(&ns, item, &len);
    assert(!ns.error);
    assert(strcmp(item->value, val) == 0);
    free(val);
    free(item);

    char* values[4] = {"value1", "value2", "value3", "value4"};
    char* keys[4] = {"01", "02", "03", "04"};
    struct kv *items[4];
    for(int i = 0; i < 4;i++){
        struct kv *it = (kv*)malloc(sizeof(struct kv));
        assert(it);
        strcpy(it->key, keys[i]);
        it->value = values[i];
        items[i] = it;
    }
    store_batch_put(&ns, items, 4);
    assert(!ns.error);
    for(int j = 0;j < 4;j++){
        free(items[j]);
    }
    struct kv *it_test = (kv*)malloc(sizeof(struct kv));
    assert(it_test);
    strcpy(it_test->key, "03");
    char *test = store_get(&ns, it_test, &len);
    assert(!ns.error);
    assert(strcmp(test, "value3") == 0);
    free(test);
    free(it_test);
    close_store(&ns);
    return 0;
}

#ifdef __cplusplus
}  /* end extern "C" */
#endif