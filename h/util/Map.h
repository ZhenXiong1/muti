/*
 * Map.h
 *
 *  Created on: 2020/9/24
 *      Author: Rick
 */

#ifndef MAP_H_
#define MAP_H_

#include "Object.h"

typedef int (*MapKeyCompare)(void *key, void *key1);
typedef uint64_t (*MapKeyHash)(void *key);

typedef struct Map Map;
typedef struct MapMethod {
        void            (*destroy)(Map *map);
        size_t          (*size)(Map *map);
        bool            (*isEmpty)(Map *map);
        void*           (*get)(Map *map, void *key);
        void*           (*put)(Map *map, void *key, void *value); /*Return replacement value*/
        void*           (*remove)(Map *map, void *key);
        void            (*clear)(Map *map, void** values); /*values must length of map size*/
} MapMethod;

struct Map {
        void            *p;
        MapMethod       *m;
};

typedef struct MapParam {
        MapKeyCompare   compareMethod;
} MapParam;

typedef struct MapHashLinkedParam {
        MapParam        super;
        uint64_t        keyOffsetInValue;
        MapKeyHash      hashMethod;
        size_t          slot_size;      /*Must 2^n*/
} MapHashLinkedParam;

bool initMapHashLinked(Map*, MapHashLinkedParam*);

#endif /* MAP_H_ */
