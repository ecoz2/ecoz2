/* memutil.c
 */

#include "utl.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


void *new_vector(int numvals, int tamval) {
    return calloc(numvals, tamval);
}

void _del_vector(void *v) {
    free(v);
}

void **new_matrix(int rows, int cols, int tamval) {
    long numbytes = (long) rows * sizeof(void *)    // rows pointers
                    + rows * cols * tamval;         // space for all the values

    void **m = (void **) malloc(numbytes);
    if (!m) {
        return 0;
    }

    char *mem = (char *) (m + rows);
    memset(mem, 0, rows * cols * tamval);
    for (int i = 0; i < rows; i++) {
        m[i] = mem;
        mem += cols * tamval;
    }
    return m;
}

void _del_matrix(void **m) {
    free(m);
}
