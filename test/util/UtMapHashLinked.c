#include <stdlib.h>
#include <stdio.h>
#include "util/Map.h"

typedef struct utMapValue {
        uint64_t value;
        uint64_t key;
} UtMapValue;

static int utInt64KeyCompare(void *key, void *key1) {
        return *(uint64_t *)key - *(uint64_t *)key1;
}

static uint64_t utInt64Hash(void *key) {
        return *(uint64_t *)key;
}

int UtMapHashLinked(int argv, char **argvs) {
        Map map;
        MapHashLinkedParam param;

        (void)argv;(void)argvs;
        param.super.compareMethod = utInt64KeyCompare;
        param.keyOffsetInValue = (long)(&((struct utMapValue *)0)->key);
        param.slot_size = 1 << 20;
        param.hashMethod = utInt64Hash;
        initMapHashLinked(&map, &param);

        uint64_t i, num = 1<<2;
        UtMapValue *v = malloc(num * sizeof(UtMapValue)), *v1;
        for (i = 0; i < num; i++) {
                v[i].value = i + 10;
                v[i].key = i;
                map.m->put(&map, &i, &v[i]);
        }

        for (i = 0; i < num; i++) {
                v1 = map.m->get(&map, &i);
                if(v1->value != i + 10) {
                        printf("Error Value key:%ld value:%ld i + 1:%ld\n", (long)v1->key, (long)v1->value, (long)i + 10);
                }
        }

        printf("Map size:%ld, isEmpty:%d\n", (long)map.m->size(&map), map.m->isEmpty(&map));
        i = 2;
        v1 = map.m->remove(&map, &i);
        printf("Map size:%ld, isEmpty:%d\n", (long)map.m->size(&map), map.m->isEmpty(&map));

        size_t sz = map.m->size(&map);
        UtMapValue **values = malloc(sizeof(void*) * sz);
        map.m->clear(&map, (void **)values);
        for (i = 0; i < sz; i++) {
                printf("%ld %ld \n", (long)values[i]->key, (long)values[i]->value);
        }

        printf("Map size:%ld, isEmpty:%d\n", (long)map.m->size(&map), map.m->isEmpty(&map));
        map.m->destroy(&map);
        free(values);
        free(v);
        fflush(stdout);
        return 0;
}
