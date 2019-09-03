/* prd.c -- ECOZ System
 */

#include "lpc.h"
#include "utl.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

Predictor *prd_create(int T, int P, const char *className) {
    Predictor *p = (Predictor *) malloc(sizeof(Predictor));
    p->vectors = (sample_t **) new_matrix(T, 1 + P, sizeof(sample_t));
    p->T = T;
    p->P = P;
    if (strlen(className) >= MAX_CLASS_NAME_LEN) {
        printf("WARN: truncating className '%s'", className);
        strncpy(p->className, className, MAX_CLASS_NAME_LEN);
        p->className[MAX_CLASS_NAME_LEN - 1] = 0;
    }
    else {
        strcpy(p->className, className);
    }
    return p;
}

void prd_destroy(Predictor *p) {
    if (p) {
        del_matrix(p->vectors);
        free(p);
    }
}

Predictor *prd_load(const char *nom_prd) {
    FILE *file;
    char className[MAX_CLASS_NAME_LEN];

    if (0 == (file = fopen(nom_prd, "r")))
        return 0;

    Predictor *predictor = 0;

    if (read_file_ident(file, "<predictor>"))
        goto done;

    if (read_class_name(file, className))
        goto done;

    int P;
    if (1 != fread(&P, sizeof(P), 1, file)) {
        goto done;
    }

    int T;
    if (1 != fread(&T, sizeof(T), 1, file)) {
        goto done;
    }

    predictor = prd_create(T, P, className);
    if (!predictor)
        goto done;

    for (int t = 0; t < T; t++) {
        sample_t *vector = predictor->vectors[t];
        if (1 != fread(vector, (1 + P) * sizeof(sample_t), 1, file)) {
            prd_destroy(predictor);
            predictor = 0;
        }
    }

    done:
    if (file) fclose(file);
    return predictor;
}

int prd_save(Predictor *predictor, char *nom_prd) {
    //printf("prd_save: %s\n", nom_prd);

    FILE *file;
    int ret = 0;

    if (0 == (file = fopen(nom_prd, "w"))) {
        printf("ERROR creating %s: %s", nom_prd, strerror(errno));
        exit(1);
        return 1;
    }

    const int P = predictor->P;
    const int T = predictor->T;

    if (write_file_ident(file, "<predictor>")) {
        ret = 2;
    }
    else if (write_class_name(file, predictor->className)) {
        ret = 2;
    }
    else if (1 != fwrite(&P, sizeof(P), 1, file)) {
        ret = 2;
    }
    else if (1 != fwrite(&T, sizeof(T), 1, file)) {
        ret = 2;
    }
    else {
        for (int t = 0; t < T; t++) {
            sample_t *vector = predictor->vectors[t];
            if (1 != fwrite(vector, (1 + P) * sizeof(sample_t), 1, file)) {
                ret = 2;
                break;
            }
        }
    }

    fclose(file);
    return ret;
}

void prd_show(Predictor *predictor) {
    printf("className='%s', T=%d, P=%d\n",
           predictor->className, predictor->T, predictor->P);

    for (int t = 0; t < predictor->T; t++) {
        sample_t *vector = predictor->vectors[t];
        printf("t=%2d: [", t);
        char *comma = "";
        for (int p = 0; p <= predictor->P; p++) {
            printf("%s%g", comma, vector[p]);
            comma = ", ";
        }
        printf("]\n");
    }
}
