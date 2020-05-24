/* hmm.classify.c -- ECOZ System
 */

#include "hmm.h"
#include "utl.h"
#include "list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <float.h>
#include <getopt.h>


static void usage() {
    printf("\
\n\
ECOZ System -- HMM based classification\n\
\n\
    hmm.classify [options] <hmm> ... <sequence> ...\n\
\n\
    hmm.classify classifies the given sequences according to given models. \n\
    Reports resulting confusion matrix and candidate order table.  \n\
\n\
    -r              show ranked models for incorrect classifications\n\
    -c <filename>   generate classification file with this name\n\
\n\
    <hmm> ...       the HMM models.\n\
\n\
    <sequence> ...  the sequences to classify.\n\
\n"
    );
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage();
        return 0;
    }
    int show_ranked = 0;
    char *classification_filename = 0;

    int opc;
    int arg;
    while (EOF != (opc = getopt(argc, argv, "rc:"))) {
        switch (opc) {
            case 'r':
                show_ranked = 1;
                break;
            case 'c':
                classification_filename = optarg;
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

    if (num_models >= MAX_MODELS) {
        fprintf(stderr, "too many models: %d\n", num_models);
        return 1;
    }

    if (arg >= argc) {
        fprintf(stderr, "sequence(s) missing.\n");
        return 1;
    }

    // beginning of the sequence arguments
    char **seq_filenames = argv + arg;
    int num_seq_filenames = argc - arg;

    if (0) {
        printf("ARGS:\n");
        printf("models: (%d)\n", num_models);
        for (int i = 0; i < num_models; ++i) {
            printf("  %s\n", model_names[i]);
        }
        printf("sequences: (%d)\n", num_seq_filenames);
        for (int i = 0; i < num_seq_filenames; ++i) {
            printf("  %s\n", seq_filenames[i]);
        }
        return 0;
    }

    return hmm_classify(
            model_names,
            num_models,
            seq_filenames,
            num_seq_filenames,
            show_ranked,
            classification_filename
            );
}
