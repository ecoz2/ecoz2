/* vq_quantize.c -- ECOZ System
 */

#include "utl.h"
#include "lpc.h"
#include "vq.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int vq_quantize(const char *nom_raas,
                const char *predictor_filenames[],
                int num_predictors,
                int show_filenames
        ) {

    Symbol *seq;

    int num_seqs;
    sample_t ddprm_total;

    int P;
    int num_raas;
    char codebook_className[MAX_CLASS_NAME_LEN];
    sample_t *reflections = cb_load((char*) nom_raas, &P, &num_raas, codebook_className);
    if (!reflections) {
        printf("error loading codebook (%d).\n", cb_errnum);
        return 1;
    }

    const int M = num_raas;

    // get raas version of the reflection vectors:
    sample_t *raas = malloc(num_raas * (1 + P) * sizeof(sample_t));
    if (!raas) {
        printf("not enough memory for codebook.\n");
        return 1;
    }
    reflections_to_raas(reflections, raas, num_raas, P);

    printf("%s: %d symbols\n", nom_raas, num_raas);

    char seq_filename[2048];

    num_seqs = 0;
    ddprm_total = 0.f;

    for (int i = 0; i < num_predictors; i++) {
        char *prd_filename = (char *) predictor_filenames[i];

        sample_t ddprm;

        Predictor *prd = prd_load(prd_filename);
        if (!prd) {
            fprintf(stderr, "%s: error loading predictor.\n", prd_filename);
            return 2;
        }

        int T = prd->T;

        if (0 == (seq = seq_create(T))) {
            printf("not enough memory for sequence.\n");
            return 1;
        }

        ddprm_total += ddprm = quantize(raas, num_raas, prd, seq);
        num_seqs++;

        char className[MAX_CLASS_NAME_LEN];
        get_class_name(prd_filename, className, sizeof(className));

        const char *base_dir = "sequences";
        if (strstr(prd_filename, "TRAIN/")) {
            base_dir = "sequences/TRAIN";
        }
        else if (strstr(prd_filename, "TEST/")) {
            base_dir = "sequences/TEST";
        }
        char base_dir_with_m[2048];
        sprintf(base_dir_with_m, "%s/M%d", base_dir, M);

        get_output_filename(prd_filename, base_dir_with_m, ".seq", seq_filename);
        if (seq_save(seq, T, num_raas, className, seq_filename)) {
            printf("error saving sequence\n");
        }

        if (show_filenames) {
            printf("\n%s className='%s' (%1.4g) = ", seq_filename, className, ddprm);
        }

        if (*className == 0) {
            printf("\nWARN%s NO className", seq_filename);
        }

        if (show_filenames) {
            seq_show(seq, T);
        }

        free(seq);
        prd_destroy(prd);
    }

    if (num_seqs > 0) {
        printf("\ntotal: %d sequences; total  average distortion = %f\n",
               num_seqs, ddprm_total / num_seqs);
    }

    return 0;
}
