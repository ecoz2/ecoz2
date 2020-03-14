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

    const int num_signals = argc - optind;
    char **sgn_filenames = argv + optind;

    return lpc_signals(
            P,
            windowLengthMs,
            offsetLengthMs,
            minpc,
            split,
            sgn_filenames,
            num_signals
    );
}
