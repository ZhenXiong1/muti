/*
 * MapHashLinked.c
 *
 *  Created on: 2020\12\5
 *      Author: Rick
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "util/Map.h"
#include "util/LinkedList.h"

typedef struct MapHashLinkedPrivate {
        MapHashLinkedParam      param;
        ListHead                *slots;
        size_t                  size;
} MapHashLinkedPrivate;

typedef struct MapHashLinkedItem {
        ListElement     element;
        void            *value;

} MapHashLinkedItem;

static size_t size(Map *this) {
        MapHashLinkedPrivate *priv = this->p;
        return priv->size;
}

static bool isEmpty(Map *this) {
        MapHashLinkedPrivate *priv = this->p;
        return priv->size == 0;
}

static void* get(Map *this, void *key) {
        MapHashLinkedPrivate *priv = this->p;
        uint64_t hash = priv->param.hashMethod(key);
        ListHead *slot = &priv->slots[hash & (priv->param.slot_size - 1)];
        MapHashLinkedItem *item;

        listForEachEntry(item, slot, element) {
                void *key1 = ((char *)item->value) + priv->param.keyOffsetInValue;
                if (priv->param.super.compareMethod(key1, key) == 0) {
                        return item->value;
                }
        }
        return NULL;
}

static void* put(Map *this, void *key, void *value) {
        MapHashLinkedPrivate *priv = this->p;
        uint64_t hash = priv->param.hashMethod(key);
        ListHead *slot = &priv->slots[hash & (priv->param.slot_size - 1)];
        void *key1 = ((char *)value) + priv->param.keyOffsetInValue;
        MapHashLinkedItem *item;
        int got = 0;
        void *ret_value = NULL;

        if(priv->param.super.compareMethod(key1, key) != 0) {
                printf("Error put key:%p", key);
                exit(-1);
        }

        listForEachEntry(item, slot, element) {
                key1 = ((char *)item->value) + priv->param.keyOffsetInValue;
                if (priv->param.super.compareMethod(key1, key) == 0) {
                        got = 1;
                        break;
                }
        }

        if (got) {
                ret_value = item->value;
        } else {
                item = malloc(sizeof(MapHashLinkedItem));
                listAddTail(&item->element, slot);
                priv->size++;
        }
        item->value = value;
        return ret_value;
}

static void* _remove(Map *this, void *key) {
        MapHashLinkedPrivate *priv = this->p;
        uint64_t hash = priv->param.hashMethod(key);
        ListHead *slot = &priv->slots[hash & (priv->param.slot_size - 1)];
        MapHashLinkedItem *item;
        int got = 0;
        void *ret_value = NULL;

        listForEachEntry(item, slot, element) {
                void *key1 = ((char *)item->value) + priv->param.keyOffsetInValue;
                if (priv->param.super.compareMethod(key1, key) == 0) {
                        got = 1;
                        break;
                }
        }

        if (got) {
                ret_value = item->value;
                listDel(&item->element);
                free(item);
                priv->size--;
        }
        return ret_value;
}

static void clear(Map *this, void** values) {
        MapHashLinkedPrivate *priv = this->p;
        MapHashLinkedItem *item, *item1;
        ListHead *slot;
        uint64_t i, j;

        j = 0;
        for (i = 0; i < priv->param.slot_size; i++) {
                slot = &priv->slots[i];
                listForEachEntrySafe(item, item1, slot, element) {
                        values[j++] = item->value;
                        listDel(&item->element);
                        free(item);
                }
        }
        priv->size = 0;
}

static void destroy(Map* this) {
        MapHashLinkedPrivate *priv = this->p;

        free(priv->slots);
        free(priv);
}

static MapMethod method = {
        .destroy = destroy,
        .size = size,
        .isEmpty = isEmpty,
        .get = get,
        .put = put,
        .remove = _remove,
        .clear = clear,
};

bool initMapHashLinked(Map* this, MapHashLinkedParam* param) {
        MapHashLinkedPrivate *priv = malloc(sizeof(*priv));
        int i;

        if ((param->slot_size & (param->slot_size-1)) != 0) {
                /*Slot size must be 2^n*/
                printf("Slot size must be 2^n\n");
                exit(-1);
        }
        this->p = priv;
        this->m = &method;
        memcpy(&priv->param, param, sizeof(*param));
        priv->slots = malloc(sizeof(ListHead) * param->slot_size);
        for (i = 0; i < param->slot_size; i++) {
                listHeadInit(&priv->slots[i]);
        }
        priv->size = 0;

        return true;
}


