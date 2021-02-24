
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

typedef int(*TestCaseFn)(int argv, char **argvs);

typedef struct TestCase {
        char *name;
        TestCaseFn fn;
} TestCase;

int UtThreadPool(int argv, char **argvs);
int UtMapHashLinked(int argv, char **argvs);
int UtSocketServer(int argv, char **argvs);
int UtSocketClient(int argv, char **argvs);
int UtSocketServer2(int argv, char **argvs);
int UtSocketClient2(int argv, char **argvs);
int UtClient(int argv, char **argvs);
int UtServer(int argv, char **argvs);
int UtClient2(int argv, char **argvs);
int UtClient3(int argv, char **argvs);

struct TestCase TestCases[] = {
        {"UtThreadPool", UtThreadPool},
        {"UtMapHashLinked", UtMapHashLinked},
        {"UtSocketServer", UtSocketServer},
        {"UtSocketClient", UtSocketClient},
        {"UtSocketServer2", UtSocketServer2},
        {"UtSocketClient2", UtSocketClient2},
        {"UtServer", UtServer},
        {"UtClient", UtClient},
	{"UtClient2", UtClient2},
	{"UtClient3", UtClient3},
        {(char *)NULL, (TestCaseFn)NULL}
};

TestCase *findTestCase(char *cnm) {
        int i = 0;
        while (TestCases[i].name != NULL) {
                if (strcasecmp(TestCases[i].name, cnm) == 0) {
                        return &TestCases[i];
                }
                i ++;
        }

        return NULL;
}

int main(int argv, char **argvs) {
        clock_t start, finish;
        int ret;
        if (argv > 1) {
                if (strcasecmp(argvs[1], "all") == 0 ) {
                        int i = 0;
                        while (TestCases[i].name != NULL) {

                                printf("\nStart test case %s ...\n", TestCases[i].name);
                                start = clock();
                                ret = (*TestCases[i].fn)(0, NULL);
                                finish = clock();

                                printf("\nEnd test case %s, Result: %d, Cost time: %ld\n", TestCases[i].name, ret, finish - start);
                                i ++;
                        }
                } else {
                    TestCase *tcase = findTestCase(argvs[1]);
                        if (tcase != NULL) {
                                start = clock();
                                ret = (*tcase->fn)(argv, argvs);
                                finish = clock();
                                printf("\nEnd test case %s, Result: %d, Cost time: %ld\n", tcase->name, ret, finish - start);
                        }
                }
        } else {
                int i = 0;
                printf("Usage:\n./application [all]|[Test Name]\nSupported Test Names:\n");
                while (TestCases[i].name != NULL) {
                        printf("%s\n", TestCases[i].name);
                        i++;
                }
        }
        return 0;
}
