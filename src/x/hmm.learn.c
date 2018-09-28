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


// maximum number of training sequences
#define MAX_SEQS 2048

// for auto convergence
static prob_t val_auto = 0.3;

// number of states and symbols
static int N = 5, M;

static int model_type = HMM_CASCADE3;

static int num_seqs;

static prob_t sum_log_prob;

static char model_className[MAX_CLASS_NAME_LEN];

static char model_filename[2048];

static int num_refinements;

static void usage() {
    printf("\
\n\
ECOZ System\n\
HMM training\n\
\n\
    hmm.learn [options] <sequence> ...\n\
\n\
    -N #          sets the number of states (%d, by default).\n\
    -I #          sets the maximum number of iterations\n\
    -a <val>      sets val_auto. (%Lg, by default)\n\
    -e <val>      sets value for epsilon restriction on B.\n\
                  0 means do not apply this restriction (%Lg, by default).    \n\
    -t <type>     sets the type of model to generate (%d, by default)\n\
                    0: random values for pi, A, and B\n\
                    1: uniform distributions\n\
                    2: cascade-2; random B\n\
                    3: cascade-3; random B\n\
\n\n",
           N, val_auto, hmm_epsilon, model_type
    );
}

static void _report_results(FILE *file) {
    fprintf(file, "\n\tModel: %s   className: '%s'\n", model_filename, model_className);
    fprintf(file, "\tN=%d M=%d type: %s\n", N, M,
           model_type == 0 ? "no restriction" :
           model_type == 1 ? "uniform distributions" :
           model_type == 2 ? "cascade-2" : "cascade-3");
    fprintf(file, "\trestriction: ");
    if ((prob_t) 0. == hmm_epsilon) fprintf(file, "No");
    else fprintf(file, "%Lg", hmm_epsilon);
    fprintf(file, "\n");
    fprintf(file, "\t        #sequences: %d\n", num_seqs);
    fprintf(file, "\t        auto value: %Lg\n", val_auto);
    fprintf(file, "\t      #refinements: %d\n", num_refinements);
    fprintf(file, "\t          Σ log(P): %Le\n", sum_log_prob);
}

static void report_results() {
    _report_results(stdout);

    char rpt_filename[2048];
    strcpy(rpt_filename, model_filename);
    camext(rpt_filename, ".rpt");
    FILE *rpt_file = fopen(rpt_filename, "w");
    if (rpt_file) {
        _report_results(rpt_file);
        fclose(rpt_file);
    }
    else {
        fprintf(stderr, "ERROR creating report %s\n", rpt_filename);
    }
}

int main(int argc, char *argv[]) {
    int opc;
    int max_iterations = -1;

    // training sequences:
    Symbol *sequences[MAX_SEQS];

    // corresponding sequence lengths:
    int T[MAX_SEQS];

    // log of probability
    prob_t prob;

    char sequence_className[MAX_CLASS_NAME_LEN];

    // model to generate:
    Hmm *hmm;

    if (argc < 2) {
        usage();
        return 0;
    }

    while (EOF != (opc = getopt(argc, argv, "N:t:e:a:I:"))) {
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
                if (sscanf(optarg, "%Lf", &hmm_epsilon) == 0) {
                    fprintf(stderr, "invalid epsilon value.\n");
                    return 1;
                }
                break;
            case 'a':
                if (sscanf(optarg, "%Lf", &val_auto) == 0) {
                    fprintf(stderr, "invalid valor auto.\n");
                    return 1;
                }
                break;
            case '?':
                return 0;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "model name missing.\n");
        return 1;
    }

    if (++optind >= argc) {
        fprintf(stderr, "no sequences given.\n");
        return 1;
    }

    if (argc - optind > MAX_SEQS) {
        fprintf(stderr, "too many training sequence (max %d).\n", MAX_SEQS);
        return 1;
    }

    // load first training sequence:

    // fprintf(stderr, "%s", argv[optind]);
    sequences[0] = seq_load(argv[optind], T, &M, sequence_className);

    if (!sequences[0]) {
        fprintf(stderr, "%s: not found.\n", argv[optind]);
        return 1;
    }

    if (M == 0) {
        fprintf(stderr, "M == 0 ! \n");
        return 1;
    }
    // use first sequence class as the one for the model:
    strcpy(model_className, sequence_className);

    char hmmDir[2048];
    if (max_iterations >= 0) {
        sprintf(hmmDir, "data/hmms/N%d__M%d_t%d__a%Lg_I%d", N, M, model_type,
                val_auto, max_iterations);
    }
    else {
        sprintf(hmmDir, "data/hmms/N%d__M%d_t%d__a%Lg", N, M, model_type,
                val_auto);
    }
    mk_dirs(hmmDir);
    sprintf(model_filename, "%s/%s.hmm", hmmDir, model_className);

    num_seqs = 1;
    for (int r = ++optind; r < argc; r++) {
        // to check all sequences come from same codebook size:
        int Mcmp;

        // fprintf(stderr, "%s", argv[r]);
        sequences[num_seqs] = seq_load(argv[r], T + num_seqs, &Mcmp, sequence_className);

        if (!sequences[num_seqs]) {
            fprintf(stderr, "%s: not found.\n", argv[r]);
        }
        else if (M != Mcmp) {
            fprintf(stderr, "%s: not conformant.\n", argv[r]);
            free(sequences[num_seqs]);
        }
        else {
            num_seqs++;
        }
    }

    prob_t log_val_auto = 0;
    prob_t abs_log_val_auto = 0;
    if (val_auto > 0) {
        log_val_auto = logl(val_auto);
        abs_log_val_auto = fabsl(log_val_auto);
    }

    fprintf(stderr, "\nN=%d M=%d type=%d  #sequences = %d\n", N, M, model_type, num_seqs);
    fprintf(stderr, "val_auto = %Lg   log=%Lg\n", val_auto, log_val_auto);
    fprintf(stderr, "max_iterations= %d\n", max_iterations);

    // create and initialize HMM:
    hmm = hmm_create(model_className, N, M);
    if (!hmm) {
        fprintf(stderr, "not enough memory for model.\n");
        return 1;
    }
    hmm_init(hmm, model_type);

    // initial B estimates per the given sequences:
    hmm_estimateB(hmm, sequences, T, num_seqs);

    const int show_probs = 0;

    if (show_probs) fprintf(stderr, "\n[Ini]: ");
    sum_log_prob = 0.;
    for (int r = 0; r < num_seqs; r++) {
        prob = hmm_log_prob(hmm, sequences[r], T[r]);
        sum_log_prob += prob;
        if (show_probs) fprintf(stderr, "%2.1Le ", prob);
    }
    if (show_probs) fprintf(stderr, "=> %Lg\n", sum_log_prob);

    // do training:

    if (hmm_refinement_prepare(hmm, sequences, T, num_seqs)) {
        fprintf(stderr, "No memory for HMM refinement.\n");
        return 2;
    }

    num_refinements = 0;
    prob_t sum_log_prob_prev = sum_log_prob;
    while (max_iterations < 0 || num_refinements < max_iterations) {
        if (hmm_refinement_step()) {
            fprintf(stderr, "refinement error\n");
            return 2;
        }
        num_refinements++;
        fprintf(stderr, ".");
        fflush(stderr);

        // probabilities post-refinement:
        if (show_probs) fprintf(stderr, "\n[%03d]: ", num_refinements);
        sum_log_prob = 0.;
        for (int r = 0; r < num_seqs; r++) {
            prob = hmm_log_prob(hmm, sequences[r], T[r]);
            sum_log_prob += prob;
            if (show_probs) fprintf(stderr, "%2.1Le ", prob);
        }
        if (show_probs) fprintf(stderr, "=> %Lg  ", sum_log_prob);

        // measure and report refinement change:
        const prob_t change = sum_log_prob - sum_log_prob_prev;
        fprintf(stderr, " %d: Δ = %+Lg  sum_log_prob = %+Lg sum_log_prob_prev = %+Lg  '%s'\n",
                num_refinements, change, sum_log_prob, sum_log_prob_prev, model_className);

        if (sum_log_prob >= 0) {
            fprintf(stderr, "\nWARNING: sum_log_prob non negative\n");
            break;
        }

        // end iterations per -a parameter?
        if (val_auto > 0 && fabsl(change) <= abs_log_val_auto) {
            break;
        }

        sum_log_prob_prev = sum_log_prob;
    }
    fprintf(stderr, "\n");

    hmm_refinement_destroy();

    report_results();

    int res = hmm_save(hmm, model_filename);
    if (res) {
        fprintf(stderr, "error saving model \"%s\": %d\n", model_filename, res);
    }

    hmm_destroy(hmm);

    return 0;
}
