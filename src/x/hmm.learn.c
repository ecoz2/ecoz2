/* hmm.learn.c  -- ECOZ System
 */

#include "hmm.h"
#include "utl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <getopt.h>


int main(int argc, char *argv[]) {
    // number of states
    int N = 5;

    int model_type = HMM_CASCADE3;

    // for auto convergence
    prob_t val_auto = 0.3;

    int max_iterations = -1;

    int seed = -1;
    int use_par = 1;

    if (argc < 2) {
        printf("\
\n\
ECOZ System\n\
HMM training\n\
\n\
    hmm.learn [options] <sequence> ...\n\
\n\
    -N #          sets the number of states (%d, by default).\n\
    -I #          sets the maximum number of iterations\n\
    -a <val>      sets val_auto. (%g, by default)\n\
    -e <val>      sets value for epsilon restriction on B.\n\
                  0 means do not apply this restriction (%g, by default).    \n\
    -t <type>     sets the type of model to generate (%d, by default)\n\
                    0: random values for pi, A, and B\n\
                    1: uniform distributions\n\
                    2: cascade-2; random B\n\
                    3: cascade-3; random B\n\
    -s <val>      Seed for random numbers. Negative means random seed (%d, by default).\n\
    -S            Use serialized impl (parallel impl, by default).\n\
    <sequence>... training sequences (max: %d)\n\
\n\n",
               N, val_auto, hmm_epsilon, model_type, seed, MAX_SEQS
        );
        return 0;
    }

    int opc;

    while (EOF != (opc = getopt(argc, argv, "N:t:e:a:I:s:S"))) {
        switch (opc) {
            case 'I':
                if (sscanf(optarg, "%u", &max_iterations) == 0) {
                    fprintf(stderr, "invalid number.\n");
                    return 1;
                }
                break;
            case 'N':
                if (sscanf(optarg, "%u", &N) == 0) {
                    fprintf(stderr, "invalid number of states.\n");
                    return 1;
                }
                break;
            case 't':
                if (sscanf(optarg, "%d", &model_type) == 0
                    || model_type < 0 || model_type > 3) {
                    fprintf(stderr, "invalid model type.\n");
                    return 1;
                }
                break;
            case 'e':
                if (sscanf(optarg, "%lf", &hmm_epsilon) == 0) {
                    fprintf(stderr, "invalid epsilon value.\n");
                    return 1;
                }
                break;
            case 'a':
                if (sscanf(optarg, "%lf", &val_auto) == 0) {
                    fprintf(stderr, "invalid valor auto.\n");
                    return 1;
                }
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

    // NOTE: I had a bug here before: was skipping the first given sequence!

    if (optind >= argc) {
        fprintf(stderr, "no sequences given.\n");
        return 1;
    }

    const unsigned num_sequences = argc - optind;

    if (num_sequences > MAX_SEQS) {
        fprintf(stderr, "too many training sequences (max %d).\n", MAX_SEQS);
        return 1;
    }

    const char **sequence_filenames = (const char **)(argv + optind);

    ecoz2_set_random_seed(seed);

    return hmm_learn(
        N,
        model_type,
        sequence_filenames,
        num_sequences,
        hmm_epsilon,
        val_auto,
        max_iterations,
        use_par,
        0  // hmm_learn_callback_t
    );
}
