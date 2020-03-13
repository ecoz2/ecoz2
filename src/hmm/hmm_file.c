/* hmm_file.c -- ECOZ System
 */

#include "hmm.h"
#include "utl.h"

#include <stdio.h>
#include <string.h>


int hmm_save(Hmm *hmm, char *filename) {
    FILE *file;

    if (0 == (file = fopen(filename, "w")))
        return 1;

    if (write_file_ident(file, "<hmm>"))
        return 2;

    if (write_class_name(file, hmm->className))
        return 3;

    if (1 != fwrite(&hmm->N, sizeof(hmm->N), 1, file))
        return 4;

    if (1 != fwrite(&hmm->M, sizeof(hmm->M), 1, file))
        return 5;

    if ((size_t) hmm->N != fwrite(hmm->pi, sizeof(prob_t), hmm->N, file))
        return 5;

    // N rows of A
    for (int i = 0; i < hmm->N; i++) {
        if ((size_t) hmm->N != fwrite(hmm->A[i], sizeof(prob_t), hmm->N, file)) {
            return 6;
        }
    }

    // N rows of B
    for (int j = 0; j < hmm->N; j++) {
        if ((size_t) hmm->M != fwrite(hmm->B[j], sizeof(prob_t), hmm->M, file)) {
            return 7;
        }
    }

    fclose(file);
    return 0;
}

Hmm *hmm_load(char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        return 0;
    }

    char className[MAX_CLASS_NAME_LEN];
    int N, M;
    Hmm *hmm = 0;

    if (read_file_ident(file, "<hmm>"))
        goto done;

    if (read_class_name(file, className)) {
        goto done;
    }

    if (1 != fread(&N, sizeof(N), 1, file)) {
        goto done;
    }

    if (1 != fread(&M, sizeof(M), 1, file)) {
        goto done;
    }

    hmm = hmm_create(className, N, M);

    if ((size_t) N != fread(hmm->pi, sizeof(prob_t), N, file)) {
        hmm_destroy(hmm);
        hmm = 0;
        goto done;
    }

    for (int i = 0; i < N; i++) {
        if ((size_t) N != fread(hmm->A[i], sizeof(prob_t), N, file)) {
            hmm_destroy(hmm);
            hmm = 0;
            goto done;
        }
    }

    for (int j = 0; j < N; j++) {
        if ((size_t) M != fread(hmm->B[j], sizeof(prob_t), M, file)) {
            hmm_destroy(hmm);
            hmm = 0;
            goto done;
        }
    }

    done:
    fclose(file);
    return hmm;
}
