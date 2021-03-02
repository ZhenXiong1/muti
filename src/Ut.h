/*
 * Ut.h
 *
 *  Created on: Mar 2, 2021
 *      Author: rick
 */

#ifndef UT_H_
#define UT_H_

typedef int(*TestCaseFn)(int argv, char **argvs);

typedef struct TestCase {
        char *name;
        TestCaseFn fn;
} TestCase;

extern TestCase TestCases[];

extern TestCase *findTestCase(char *cnm);

extern int UtDo(int argv, char **argvs);

#endif /* UT_H_ */
