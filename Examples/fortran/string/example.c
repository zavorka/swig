/* File : example.c */
#include <stdio.h>

void sayhi(char *str, char *ret) {
    if (ret != NULL) {
        sprintf(ret, "hello %s", str);
    }
    return;
}
