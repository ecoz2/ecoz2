/* vq.quantize.c -- ECOZ System
 */

#include "utl.h"
#include "lpc.h"
#include "vq.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

static void usage() {
    printf("\
\n\
ECOZ System\n\
Vector quantization\n\
\n\
	vq.quantize <codebook> <predictor> ...\n\
\n\
	<codebook>	 reference codebook for quantization.\n\
\n\
	<predictor> ...	LPC vector sequences to be quantized.\n\
\n\n"
    );
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        usage();
        return 0;
    }

    char *nom_raas = argv[1];

    Symbol *seq;

    int num_cads;
    sample_t ddprm_total;

    int P;
    int num_raas;
    char codebook_className[MAX_CLASS_NAME_LEN];
    sample_t *reflections = cb_load(nom_raas, &P, &num_raas, codebook_className);
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

    num_cads = 0;
    ddprm_total = 0.f;
    for (int opt = 2; opt < argc; opt++) {
        sample_t ddprm;

        char *prd_filename = argv[opt];
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
        num_cads++;

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

        printf("\n%s className='%s' (%1.4g) = ", seq_filename, className, ddprm);

        if (*className == 0) {
            printf("\nWARN%s NO className", seq_filename);
        }

        seq_show(seq, T);
        free(seq);
        prd_destroy(prd);
    }

    if (num_cads > 0) {
        printf("\ntotal: %d sequences; dprm total = %f\n",
               num_cads, ddprm_total / num_cads);
    }

    return 0;
}
