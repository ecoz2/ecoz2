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

prob_t ***new_matrix3(int d0, int d1, int d2) {
    prob_t ***m = (prob_t ***) malloc(d0 * sizeof(prob_t **));
    if (!m) {
        return 0;
    }

    for (int i = 0; i < d0; ++i) {
        m[i] = (prob_t **) malloc(d1 * sizeof(prob_t *));
        if (!m[i]) {
            del_matrix3(m, d0, d1);
            return 0;
        }

        for (int j = 0; j < d1; ++j) {
            m[i][j] = (prob_t *) malloc(d2 * sizeof(prob_t));
            if (!m[i][j]) {
                del_matrix3(m, d0, d1);
                return 0;
            }
        }
    }
    return m;
}

void del_matrix3(prob_t ***m, int d0, int d1) {
    if (m) {
        for (int i = d0 - 1; i >= 0; --i) {
            if (m[i]) {
                for (int j = d1 - 1; j >= 0; --j) {
                    if (m[i][j]) {
                        free(m[i][j]);
                    }
                }
                free(m[i]);
            }
        }
        free(m);
    }
}
