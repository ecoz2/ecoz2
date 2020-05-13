/* vq.learn.c -- ECOZ System
 * Main program for codebook generation.
 */

#include <string.h>
#include <getopt.h>

#include "vq.h"

static void usage(sample_t epsilon, int seed) {
    printf("\n\
ECOZ System\n\
Codebook training\n\
\n\
    vq.learn [options] <predictor> ...\n\
\n\
    -B <codebook>   Start training from this base codebook.\n\
    -P <val>        Prediction order (required if -B not given).\n\
    -w <id>         Class ID to associate to generated codebooks.\n\
    -e <val>        Epsilon parameter for convergence (%g, by default).\n\
    -s <val>        Seed for random numbers. Negative means random seed (%d, by default).\n\
    -S              Use serialized impl (parallel impl, by default).\n\
    <predictor>...  training predictor files\n\
\n\n",
           epsilon, seed
    );
}

int main(int argc, char *argv[]) {
    increment_stack_size();

    char base_codebook[2048] = "";
    int P = 0;
    sample_t epsilon = .05;
    char codebook_className[MAX_CLASS_NAME_LEN] = "_";

    int seed = -1;
    int use_par = 1;

    if (argc < 2) {
        usage(epsilon, seed);
        return 0;
    }

    int opc;
    while (EOF != (opc = getopt(argc, argv, "B:P:e:w:s:S"))) {
        switch (opc) {
            case 'B':
                strcpy(base_codebook, optarg);
                break;
            case 'P':
                if (sscanf(optarg, "%d", &P) == 0 || P <= 0) {
                    fprintf(stderr, "invalid -P value.\n");
                    return 1;
                }
                break;
            case 'e':
                if (sscanf(optarg, "%lf", &epsilon) == 0) {
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
            case 'S':
                use_par = 0;
                break;
            case '?':
                return 0;
        }
    }

    if (base_codebook[0]) {
        if (P > 0) {
            fprintf(stderr, "-B and -P are mutually exclusive.\n");
            return 1;
        }
    }
    else {
        if (P <= 0) {
            fprintf(stderr, "missing -P parameter.\n");
            return 1;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "missing predictor files\n");
        return 1;
    }

    ecoz2_set_random_seed(seed);

    const int numPredictors = argc - optind;
    char **predictor_filenames = argv + optind;

    if (base_codebook[0]) {
        return vq_learn_using_base_codebook(
                base_codebook,
                epsilon,
                (const char **) predictor_filenames,
                numPredictors,
                use_par,
                0  // callback
        );
    }
    else {
        return vq_learn(
                P,
                epsilon, codebook_className,
                (const char **) predictor_filenames,
                numPredictors,
                use_par,
                0  // callback
        );
    }
}
