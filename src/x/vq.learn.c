/* vq.learn.c -- ECOZ System
 * Main program for codebook generation.
 */

#include <string.h>
#include <getopt.h>

#include "vq.h"

static void usage(sample_t eps, int seed) {
    printf("\n\
ECOZ System\n\
Codebook training\n\
\n\
    vq.learn -P <prediction-order> [options] <predictor> ...\n\
\n\
    -w <className>  Sets the className ID to associate to codebook.\n\
    -e <epsilon>    Sets the epsilon parameter for convergence (%g, by default).\n\
    -s <val>        Seed for random numbers. Negative means random seed (%d, by default).\n\
    <predictor>...  training predictor files\n\
\n\n",
    eps, seed
    );
}

int main(int argc, char *argv[]) {
    int P = 0;
    sample_t eps = .05;
    char codebook_className[MAX_CLASS_NAME_LEN] = "_";

    int seed = -1;

    if (argc < 2) {
        usage(eps, seed);
        return 0;
    }

    int opc;
    while (EOF != (opc = getopt(argc, argv, "P:e:w:s:"))) {
        switch (opc) {
            case 'P':
                if (sscanf(optarg, "%d", &P) == 0 || P <= 0) {
                    fprintf(stderr, "invalid -P value.\n");
                    return 1;
                }
                break;
            case 'e':
                if (sscanf(optarg, "%lf", &eps) == 0) {
                    printf("invalid epsilon\n");
                    return 1;
                }
                break;
            case 'w':
                if (strlen(optarg) >= MAX_CLASS_NAME_LEN) {
                    printf("class name to long");
                    return 1;
                }
                strcpy(codebook_className, optarg);
                break;
            case 's':
                if (sscanf(optarg, "%d", &seed) == 0) {
                    fprintf(stderr, "invalid seed.\n");
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
        printf("predictors not provided\n");
        return 1;
    }

    ecoz2_set_random_seed(seed);

    const int numPredictors = argc - optind;
    char **predictor_filenames = argv + optind;

    return vq_learn(P, eps, codebook_className,
                    (const char **) predictor_filenames,
                    numPredictors,
                    0  // callback
    );
}
