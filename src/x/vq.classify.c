/* vq.classify.c -- ECOZ System
 */

#include "vq.h"
#include "utl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <float.h>
#include <getopt.h>


static void usage() {
    printf("\
\n\
ECOZ System -- VQ based classification\n\
\n\
    vq.classify [-r] <codebook> ... <predictor> ...\n\
\n\
    vq.classify classifies the given predictor files according to given models. \n\
    Reports resulting confusion matrix and candidate order table.  \n\
\n\
    -r    show ranked models for incorrect classifications\n\
\n\
    <codebook> ...  the models.\n\
\n\
    <predictor> ... the predictor files to classify.\n\
\n"
    );
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage();
        return 0;
    }
    int show_ranked = 0;

    int opc;
    int arg;

    while (EOF != (opc = getopt(argc, argv, "r"))) {
        switch (opc) {
            case 'r':
                show_ranked = 1;
                break;
            case '?':
                return 0;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "models missing.\n");
        return 1;
    }

    // beginning of the model arguments
    char **model_names = argv + optind;
    int num_models = 0;
    for (arg = optind; arg < argc; arg++) {
        char *model_name = argv[arg];
        if (ends_with(model_name, ".hmm")) {
            ++num_models;
        }
        else break;
    }

    if (num_models >= MAX_CB_MODELS) {
        fprintf(stderr, "too many models: %d\n", num_models);
        return 1;
    }

    if (arg >= argc) {
        fprintf(stderr, "sequence(s) missing.\n");
        return 1;
    }

    // beginning of the predictor arguments
    char **prd_filenames = argv + arg;
    int num_prd_filenames = argc - arg;

    if (0) {
        printf("ARGS:\n");
        printf("models: (%d)\n", num_models);
        for (int i = 0; i < num_models; ++i) {
            printf("  %s\n", model_names[i]);
        }
        printf("predictors: (%d)\n", num_prd_filenames);
        for (int i = 0; i < num_prd_filenames; ++i) {
            printf("  %s\n", prd_filenames[i]);
        }
        return 0;
    }

    return vq_classify(
        model_names,
        num_models,
        prd_filenames,
        num_prd_filenames,
        show_ranked
    );
}
