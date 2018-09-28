/* lpc.c -- ECOZ System
 */

#include "lpc.h"

#include "sgn.h"
#include "utl.h"
#include "list.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <assert.h>

static int default_windowLengthMs = 45;
static int default_offsetLengthMs = 15;

static void usage() {
    printf("\
\n\
ECOZ System\n\
Linear Prediction Coding\n\
\n\
    lpc -P <prediction-order> [options] <signal> ...\n\
\n\
    -W <ms>     Analysis window length in milliseconds (%d)\n\
    -O <ms>     Offset length in milliseconds (%d)\n\
    -m #        Only process a class if it has at least this number of signals\n\
    -s <split>  Put the generated predictors into two different training\n\
                and test subsets (with the given approx ratio)\n\
\n",
           default_windowLengthMs, default_offsetLengthMs
    );
};

typedef struct {
    char className[MAX_CLASS_NAME_LEN];
    List sgn_filenames;
} ByClass;

static ByClass *findOrCreate(List byClasses, const char *className) {
    ByClass *e;
    long numClasses = list_size(byClasses);
    for (long i = 0; i < numClasses; i++) {
        e = (ByClass *) list_elementAt(byClasses, i);
        if (0 == strcmp(e->className, className)) {
            return e;
        }
    }
    e = (ByClass *) malloc(sizeof(ByClass));
    assert(e);
    strcpy(e->className, className);
    e->sgn_filenames = list_create();
    list_addElement(byClasses, e);
    return e;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage();
        return 0;
    }
    int P = 0;
    int windowLengthMs = default_windowLengthMs;
    int offsetLengthMs = default_offsetLengthMs;

    int minpc = 0;
    float split = 0;

    // predictor to be generated:
    char prd_filename[2048];

    int opc;
    while (EOF != (opc = getopt(argc, argv, "P:W:O:m:s:"))) {
        switch (opc) {
            case 'P':
                if (sscanf(optarg, "%d", &P) == 0 || P <= 0) {
                    fprintf(stderr, "invalid -P value.\n");
                    return 1;
                }
                break;
            case 'W':
                if (sscanf(optarg, "%d", &windowLengthMs) == 0 || windowLengthMs <= 0) {
                    fprintf(stderr, "invalid -W value.\n");
                    return 1;
                }
                break;
            case 'O':
                if (sscanf(optarg, "%d", &offsetLengthMs) == 0 || offsetLengthMs <= 0) {
                    fprintf(stderr, "invalid -O value.\n");
                    return 1;
                }
                break;
            case 'm':
                if (sscanf(optarg, "%d", &minpc) == 0 || minpc < 0) {
                    fprintf(stderr, "invalid -m value.\n");
                    return 1;
                }
                break;
            case 's':
                if (sscanf(optarg, "%f", &split) == 0 || split <= 0 || split >= 1) {
                    fprintf(stderr, "invalid split value.\n");
                    return 1;
                }
                break;
            case '?':
                return 0;
        }
    }

    if (P <= 0) {
        fprintf(stderr, "missing -P parameter.\n");
        return 1;
    }
    if (optind >= argc) {
        printf("missing signal\n");
        return 1;
    }

    List byClasses = list_create();

    for (int i = optind; i < argc; i++) {
        char *sgn_filename = argv[i];
        char className[MAX_CLASS_NAME_LEN] = "";
        get_class_name(sgn_filename, className, sizeof(className));
        ByClass *e = findOrCreate(byClasses, className);
        list_addElement(e->sgn_filenames, sgn_filename);
    }

    // process the collected classes:

    printf("Number of classes: %ld\n", list_size(byClasses));

    const long numClasses = list_size(byClasses);
    for (long i = 0; i < numClasses; i++) {
        ByClass *e = (ByClass *) list_elementAt(byClasses, i);

        list_shuffle(e->sgn_filenames);
        long numSignals = list_size(e->sgn_filenames);

        if (minpc > 0 && numSignals < minpc) {
            printf("class '%s': insufficient #signals=%ld\n", e->className, numSignals);
            continue;
        }

        int numTrain = -1;
        if (split > 0) {
            numTrain = (int) (split * numSignals);
        }

        printf("class '%s': %ld\n", e->className, numSignals);

        for (long j = 0; j < numSignals; j++) {
            char *sgn_filename = (char *) list_elementAt(e->sgn_filenames, j);
            printf("  %s\n", sgn_filename);

            Sgn *sgn = sgn_load(sgn_filename);
            if (!sgn) {
                printf("%s: error loading signal\n", sgn_filename);
                continue;
            }

            //printf("\n<%s>:\n", sgn_filename);
            //sgn_show(sgn);

            Predictor *predictor = lpaOnSignal(P, windowLengthMs, offsetLengthMs, sgn);
            if (!predictor) {
                sgn_destroy(sgn);
                printf("cannot create lpc predictor\n");
                continue;
            }

            const char *base_dir = "predictors";
            if (numTrain > 0) {
                if (j <= numTrain) {
                    base_dir = "predictors/TRAIN";
                }
                else {
                    base_dir = "predictors/TEST";
                }
            }
            get_output_filename(sgn_filename, base_dir, ".prd", prd_filename);

            //printf("<%s>:\n", prd_filename);

            strcpy(predictor->className, e->className);

            if (prd_save(predictor, prd_filename)) {
                printf("%s: error saving predictor\n", prd_filename);
            }
            else {
                printf("%s: '%s': predictor saved\n", prd_filename, predictor->className);
            }

            prd_destroy(predictor);
            sgn_destroy(sgn);
            printf("\n");
        }
    }

    for (long i = 0; i < numClasses; i++) {
        ByClass *e = (ByClass *) list_elementAt(byClasses, i);
        list_destroy(e->sgn_filenames);
    }
    list_destroy(byClasses);

    return 0;
}
