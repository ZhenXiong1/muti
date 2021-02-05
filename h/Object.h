/*
 * Object.h
 *
 *  Created on: 2020/9/22
 *      Author: Rick
 */

#ifndef OBJECT_H_
#define OBJECT_H_

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct Object Object;
typedef struct ObjectMethod {
	void	(*destroy)(Object*);
} ObjectMethod;

struct Object {
	void		*p;
	ObjectMethod	*m;
};


#endif /* OBJECT_H_ */
