/* vq.c
 */

#include "vq.h"
#include "lpc.h"
#include "utl.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>


////////////////////////////////////////////////////////////////////
// See below new interface for loading already created codebooks.


// error code in operations:

int cb_errnum = 0;

int cb_save(const char *className,
            sample_t *reflections, int num_vecs, int P, char *filename) {

    FILE *file = fopen(filename, "w");
    if (!file) {
        return cb_errnum = 1;
    }

    if (write_file_ident(file, "<codebook>")) {
        fclose(file);
        return cb_errnum = 1;
    }

    if (write_class_name(file, className)) {
        fclose(file);
        return cb_errnum = 1;
    }

    if (1 != fwrite(&P, sizeof(P), 1, file)) {
        fclose(file);
        return cb_errnum = 1;
    }

    if (1 != fwrite(&num_vecs, sizeof(num_vecs), 1, file)) {
        fclose(file);
        return cb_errnum = 1;
    }

    sample_t *entr = reflections;
    for (int i = 0; i < num_vecs; i++, entr += (1 + P)) {
        if (1 != fwrite(entr, (1 + P) * sizeof(sample_t), 1, file)) {
            fclose(file);
            return cb_errnum = 1;
        }
    }

    fclose(file);
    return cb_errnum = 0;
}

sample_t *cb_load(char *filename, int *P, int *num_vecs, char *className) {

    FILE *file = fopen(filename, "r");
    if (!file) {
        cb_errnum = 1;
        return 0;
    }

    if (read_file_ident(file, "<codebook>")) {
        fclose(file);
        cb_errnum = 2;
        return 0;
    }

    if (read_class_name(file, className)) {
        fclose(file);
        cb_errnum = 2;
        return 0;
    }

    if (1 != fread(P, sizeof(*P), 1, file)) {
        fclose(file);
        cb_errnum = 3;
        return 0;
    }

    if (1 != fread(num_vecs, sizeof(*num_vecs), 1, file)) {
        fclose(file);
        cb_errnum = 3;
        return 0;
    }

    long mem_size = (long) *num_vecs * (1 + *P) * sizeof(sample_t);

    sample_t *reflections = (sample_t *) malloc(mem_size);
    if (!reflections) {
        cb_errnum = 3;
        fclose(file);
        return 0;
    }

    sample_t *vec = reflections;
    for (int i = 0; i < *num_vecs; i++, vec += (1 + *P)) {
        if (1 != fread(vec, (1 + *P) * sizeof(sample_t), 1, file)) {
            fclose(file);
            cb_errnum = 4;
            free(reflections);
            return 0;
        }
    }
    fclose(file);

    cb_errnum = 0;
    return reflections;
}


////////////////////////////////////////////////////////////////////
// new scheme.  Now only for loading already created codebooks


Codebook *cbook_load(char *filename) {
    Codebook *cbook = (Codebook *) calloc(1, sizeof(Codebook));
    if (!cbook)
        return 0;

    cbook->reflections = cb_load(filename, &cbook->P, &cbook->num_vecs, cbook->className);
    if (!cbook->reflections) {
        cbook_destroy(cbook);
        return 0;
    }

    const int P = cbook->P;

    cbook->raas = (sample_t *) calloc(cbook->num_vecs, (1 + P) * sizeof(sample_t));
    if (!cbook->raas) {
        cbook_destroy(cbook);
        return 0;
    }
    reflections_to_raas(cbook->reflections, cbook->raas, cbook->num_vecs, cbook->P);

    return cbook;
}

void cbook_destroy(Codebook *cbook) {
    if (cbook->raas)
        free(cbook->raas);
    if (cbook->reflections)
        free(cbook->reflections);
    free(cbook);
}

sample_t cbook_quantize(Codebook *cbook, Predictor *prd, Symbol *seq) {
    assert(cbook->P == prd->P);
    return quantize(cbook->raas, cbook->num_vecs, prd, seq);
}
