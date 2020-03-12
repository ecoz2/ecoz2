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


static int ends_with(char* filename, char* str) {
    const int len_i = strlen(filename);
    const int len_j = strlen(str);
    if (len_i < len_j) {
        return 0;
    }
    const int num_checks = len_j;
    int i = len_i - 1;
    int j = len_j - 1;
    for (int k = 0; k < num_checks; ++k) {
        if (filename[i] != str[j]) {
            return 0;
        }
    }
    return 1;
}

static void usage() {
    printf("\
\n\
ECOZ System -- HMM based classification\n\
\n\
    hmm.classify [-r] <hmm> ... <sequence> ...\n\
\n\
    hmm.classify classifies the given sequences according to given models. \n\
    Reports resulting confusion matrix and candidate order table.  \n\
\n\
    -r    show ranked models for incorrect classifications\n\
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
        printf("models: (%d)\n", num_seq_filenames);
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
            show_ranked
            );
}
