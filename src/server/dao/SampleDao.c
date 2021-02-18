/*
 * SampleDao.c
 *
 *  Created on: Feb 2, 2021
 *      Author: root
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <server/dao/SampleDao.h>
#include <util/Map.h>
#include <Log.h>

typedef struct SampleDaoPrivate {
        SampleDaoParam          param;
        Map                     map;
        ListHead                list;
        int                     list_size;
} SampleDaoPrivate;

static Sample* getSample(SampleDao* this, int id) {
        SampleDaoPrivate *priv_p = this->p;
        Map *map = &priv_p->map;
        return map->m->get(map, &id);
}

static int putSample(SampleDao* this, Sample *sample) {
        SampleDaoPrivate *priv_p = this->p;
        Map *map = &priv_p->map;
        Sample *s = map->m->get(map, &sample->id);
        if (s == NULL) {
                s = malloc(sizeof(*s) + sample->path_length);
                memcpy(s, sample, sizeof(*s) + sample->path_length);
                map->m->put(map, &s->id, s);
                listAddTail(&s->element, &priv_p->list);
                priv_p->list_size++;
                DLOG("Add new sample, id:%d", s->id);
        } else {
                memcpy(s, sample, sizeof(*s) + sample->path_length);
                DLOG("Replace old sample, id:%d", s->id);
        }
        return 0;
}

#define min(a, b) (a) < (b) ? (a) : (b)

static int listSample(SampleDao* this, ListHead **head, int page, int page_size) {
        SampleDaoPrivate *priv_p = this->p;
        int offset = page * page_size;
        int off = 0;
        Sample *s;

        if (offset >= priv_p->list_size) return 0;
        int left = priv_p->list_size - offset;
        listForEachEntry(s, &priv_p->list, element) {
                if (offset == off) {
                        break;
                }
                off++;
        }
        *head = &s->element;
        return min(left, page_size);
}

static void destroy(SampleDao* this) {
        SampleDaoPrivate *priv_p = this->p;
        Map *map = &priv_p->map;
        Sample **values = malloc(sizeof(void*) * priv_p->list_size);
        int i;

        listHeadInit(&priv_p->list);
        map->m->clear(map, (void **)values);
        for (i = 0; i < priv_p->list_size; i++) {
                free(values[i]);
        }
        map->m->destroy(map);
        free(priv_p);
}

static SampleDaoMethod method = {
        .getSample = getSample,
        .putSample = putSample,
        .listSample = listSample,
        .destroy = destroy,
};

static int SampleCompareMethod(void *key, void *key1) {
        return (*(int *)key) - (*(int *)key1);
}

static uint64_t SampleHashMethod(void *key) {
        return (uint64_t)(*(int *)key);
}

bool initSampleDao(SampleDao* this, SampleDaoParam* param) {
        MapHashLinkedParam map_param;
        bool rc;
        SampleDaoPrivate *priv_p = malloc(sizeof(*priv_p));

        this->p = priv_p;
        this->m = &method;
        memcpy(&priv_p->param, param, sizeof(*param));

        listHeadInit(&priv_p->list);

        map_param.super.compareMethod = SampleCompareMethod;
        map_param.hashMethod = SampleHashMethod;
        map_param.keyOffsetInValue = (uint64_t)&((Sample*)NULL)->id;
        map_param.slot_size = 1 << 7;
        rc = initMapHashLinked(&priv_p->map, &map_param);
        if (rc == false) {
                free(priv_p);
                goto out;
        }

out:
        return rc;
}


