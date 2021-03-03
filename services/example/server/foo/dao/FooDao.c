/*
 * FooDao.c
 *
 *  Created on: Feb 2, 2021
 *      Author: root
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include "FooDao.h"
#include <util/Map.h>
#include <Log.h>

typedef struct FooDaoPrivate {
        FooDaoParam          param;
        Map                     map;
        ListHead                list;
        int                     list_size;
        pthread_mutex_t         lock;
} FooDaoPrivate;

static Foo* getFoo(FooDao* this, int id) {
        FooDaoPrivate *priv_p = this->p;
        Map *map = &priv_p->map;
        Foo* s;
        pthread_mutex_lock(&priv_p->lock);
        s = map->m->get(map, &id);
        pthread_mutex_unlock(&priv_p->lock);
        return s;
}

static int putFoo(FooDao* this, Foo *foo) {
        FooDaoPrivate *priv_p = this->p;
        Map *map = &priv_p->map;
        Foo *s;

        pthread_mutex_lock(&priv_p->lock);
        s = map->m->get(map, &foo->id);
        if (s == NULL) {
                s = malloc(sizeof(*s) + foo->path_length);
                memcpy(s, foo, sizeof(*s) + foo->path_length);
                map->m->put(map, &s->id, s);
                listAddTail(&s->element, &priv_p->list);
                priv_p->list_size++;
//                DLOG("Add new foo, id:%d", s->id);
        } else {
                memcpy(s, foo, sizeof(*s) + foo->path_length);
//                DLOG("Replace old foo, id:%d", s->id);
        }
        pthread_mutex_unlock(&priv_p->lock);
        return 0;
}

#define min(a, b) (a) < (b) ? (a) : (b)

static int listFoo(FooDao* this, ListHead **head, int page, int page_size) {
        FooDaoPrivate *priv_p = this->p;
        int offset = page * page_size;
        int off = 0;
        Foo *s;

        if (offset >= priv_p->list_size) return 0;
        pthread_mutex_lock(&priv_p->lock);
        int left = priv_p->list_size - offset;
        listForEachEntry(s, &priv_p->list, element) {
                if (offset == off) {
                        break;
                }
                off++;
        }
        *head = &s->element;
        pthread_mutex_unlock(&priv_p->lock);
        return min(left, page_size);
}

static void destroy(FooDao* this) {
        FooDaoPrivate *priv_p = this->p;
        Map *map = &priv_p->map;
        Foo **values;
        int i;

        if (priv_p->list_size) {
        	pthread_mutex_lock(&priv_p->lock);
                assert(map->m->size(map) == priv_p->list_size);
                values = malloc(sizeof(void*) * priv_p->list_size);
                listHeadInit(&priv_p->list);
                map->m->clear(map, (void **)values);
                for (i = 0; i < priv_p->list_size; i++) {
                        free(values[i]);
                }
                free(values);
                pthread_mutex_unlock(&priv_p->lock);
        }
        map->m->destroy(map);

        free(priv_p);
}

static FooDaoMethod method = {
        .getFoo = getFoo,
        .putFoo = putFoo,
        .listFoo = listFoo,
        .destroy = destroy,
};

static int FooCompareMethod(void *key, void *key1) {
        return (*(int *)key) - (*(int *)key1);
}

static uint64_t FooHashMethod(void *key) {
        return (uint64_t)(*(int *)key);
}

bool initFooDao(FooDao* this, FooDaoParam* param) {
        MapHashLinkedParam map_param;
        bool rc;
        FooDaoPrivate *priv_p = malloc(sizeof(*priv_p));

        this->p = priv_p;
        this->m = &method;
        memcpy(&priv_p->param, param, sizeof(*param));

        listHeadInit(&priv_p->list);
        priv_p->list_size = 0;
        pthread_mutex_init(&priv_p->lock, NULL);

        map_param.super.compareMethod = FooCompareMethod;
        map_param.hashMethod = FooHashMethod;
        map_param.keyOffsetInValue = (uint64_t)&((Foo*)NULL)->id;
        map_param.slot_size = 1 << 20;
        rc = initMapHashLinked(&priv_p->map, &map_param);
        if (rc == false) {
                free(priv_p);
                goto out;
        }

out:
        return rc;
}


