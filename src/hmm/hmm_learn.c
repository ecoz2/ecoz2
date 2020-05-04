/* hmm.learn.c  -- ECOZ System
 */

#include "hmm.h"
#include "utl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <assert.h>

#include "hmm_refinement.c"

#include <omp.h>

// for auto convergence
static prob_t val_auto = 0.3;
static prob_t log_val_auto;
static prob_t abs_log_val_auto;

// number of states and symbols
static int N = 5, M;

static int model_type = HMM_CASCADE3;

static int num_seqs;

static prob_t sum_log_prob;

static char model_className[MAX_CLASS_NAME_LEN];

static char model_filename[2048];

static int num_refinements;


static void _report_results(FILE *file) {
    fprintf(file, "\n\tModel: %s   className: '%s'\n", model_filename, model_className);
    fprintf(file, "\tN=%d M=%d type: %s\n", N, M,
           model_type == 0 ? "no restriction" :
           model_type == 1 ? "uniform distributions" :
           model_type == 2 ? "cascade-2" : "cascade-3");
    fprintf(file, "\trestriction: ");
    if ((prob_t) 0. == hmm_epsilon) fprintf(file, "No");
    else fprintf(file, "%g", hmm_epsilon);
    fprintf(file, "\n");
    fprintf(file, "\t        #sequences: %d\n", num_seqs);
    fprintf(file, "\t        auto value: %g  (|log|=%g)\n", val_auto, abs_log_val_auto);
    fprintf(file, "\t      #refinements: %d\n", num_refinements);
    fprintf(file, "\t          Σ log(P): %e\n", sum_log_prob);
}

static void report_results(void) {
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

static inline prob_t get_sum_log_prob(Hmm *hmm, int num_seqs, Symbol **sequences, int *T, int use_par) {
    const double measure_start_sec = measure_time_now_sec();
    prob_t sum_log_prob = 0.;

    if (use_par) {
        #pragma omp parallel for reduction (+:sum_log_prob)
        for (int r = 0; r < num_seqs; ++r) {
            prob_t prob = hmm_log_prob(hmm, sequences[r], T[r]);
            sum_log_prob += prob;
        }
    }
    else {
        for (int r = 0; r < num_seqs; ++r) {
            prob_t prob = hmm_log_prob(hmm, sequences[r], T[r]);
            sum_log_prob += prob;
        }
    }

    printf("get_sum_log_prob took %s\n", measure_time_show_elapsed(measure_time_now_sec() - measure_start_sec));
    return sum_log_prob;
}


int hmm_learn(
        int N_,
        int model_type_,
        const char* sequence_filenames[],
        int num_sequences,
        double hmm_epsilon_,
        double val_auto_,
        int max_iterations,
        int use_par,
        hmm_learn_callback_t callback
        ) {

    assert(num_sequences > 0);
    assert(num_sequences <= MAX_SEQS);

    fprintf(stderr, "hmm_learn: num_sequences = %d\n", num_sequences);

    N = N_;
    model_type = model_type_;
    hmm_epsilon = hmm_epsilon_;
    val_auto = val_auto_;


    // training sequences:
    Symbol *sequences[MAX_SEQS];

    // corresponding sequence lengths:
    int T[MAX_SEQS];

    char sequence_className[MAX_CLASS_NAME_LEN];

    // model to generate:
    Hmm *hmm;

    // load first training sequence:

    fprintf(stderr, "  %3d: %s\n", 0, sequence_filenames[0]);
    sequences[0] = seq_load(sequence_filenames[0], T, &M, sequence_className);

    if (!sequences[0]) {
        fprintf(stderr, "%s: not found.\n", sequence_filenames[0]);
        return 1;
    }

    if (M == 0) {
        fprintf(stderr, "M == 0 ! \n");
        return 1;
    }

    int max_T = T[0];

    // use first sequence class as the one for the model:
    strcpy(model_className, sequence_className);

    char hmmDir[2048];
    if (max_iterations >= 0) {
        sprintf(hmmDir, "data/hmms/N%d__M%d_t%d__a%g_I%d", N, M, model_type,
                val_auto, max_iterations);
    }
    else {
        sprintf(hmmDir, "data/hmms/N%d__M%d_t%d__a%g", N, M, model_type,
                val_auto);
    }
    mk_dirs(hmmDir);
    #pragma GCC diagnostic ignored "-Wformat-overflow"
    sprintf(model_filename, "%s/%s.hmm", hmmDir, model_className);

    num_seqs = 1;
    for (int r = 1; r < num_sequences; ++r) {
        // to check all sequences come from same codebook size:
        int Mcmp;

        int show_filename = 0;
        if (num_sequences > 8) {
            if (r == 3) {
                fprintf(stderr, "  ...\n");
            }
            else if (r < 3 || r > num_sequences - 3 - 1) {
                show_filename = 1;
            }
        }
        else show_filename = 1;
        if (show_filename) {
            fprintf(stderr, "  %3d: %s\n", r, sequence_filenames[r]);
        }

        sequences[num_seqs] = seq_load(sequence_filenames[r], T + num_seqs, &Mcmp, sequence_className);

        if (!sequences[num_seqs]) {
            fprintf(stderr, "%s: not found.\n", sequence_filenames[r]);
        }
        else if (M != Mcmp) {
            fprintf(stderr, "%s: not conformant.\n", sequence_filenames[r]);
            free(sequences[num_seqs]);
        }
        else {
            if (max_T < T[num_seqs]) {
                max_T = T[num_seqs];
            }
            num_seqs++;
        }
    }

    if (val_auto > 0) {
        log_val_auto = logl(val_auto);
        abs_log_val_auto = fabsl(log_val_auto);
    }

    fprintf(stderr, "\nN=%d M=%d type=%d  #sequences = %d  max_T=%d\n", N, M, model_type, num_seqs, max_T);
    fprintf(stderr, "val_auto = %g   log=%g   max_iterations=%d\n", val_auto, log_val_auto, max_iterations);

    const double measure_start_sec = measure_time_now_sec();

    // create and initialize HMM:
    hmm = hmm_create(model_className, N, M);
    if (!hmm) {
        fprintf(stderr, "not enough memory for model.\n");
        return 1;
    }
    hmm_init(hmm, model_type);

    // initial B estimates per the given sequences:
    hmm_estimateB(hmm, sequences, T, num_seqs, max_T);

    sum_log_prob = get_sum_log_prob(hmm, num_seqs, sequences, T, use_par);

    // do training:

    if (hmm_refinement_prepare(hmm, sequences, T, num_seqs)) {
        fprintf(stderr, "No memory for HMM refinement.\n");
        return 2;
    }
    printf("refinement info prepared\n");

    num_refinements = 0;
    prob_t sum_log_prob_prev = sum_log_prob;
    while (max_iterations < 0 || num_refinements < max_iterations) {

        const double measure_ref_start_sec = measure_time_now_sec();

        if (hmm_refinement_step()) {
            fprintf(stderr, "refinement error\n");
            return 2;
        }
        num_refinements++;
        fprintf(stderr, ".");
        fflush(stderr);

        // probabilities post-refinement:
        sum_log_prob = get_sum_log_prob(hmm, num_seqs, sequences, T, use_par);

        // measure and report refinement change:
        const prob_t change = sum_log_prob - sum_log_prob_prev;

        fprintf(stderr, " %3d: Δ = %+10.6g  sum_log_prob = %+10.6g  prev = %+10.6g  '%s'  (%.3fs)\n",
                num_refinements, change, sum_log_prob, sum_log_prob_prev, model_className,
                measure_time_now_sec() - measure_ref_start_sec
                );

        if (sum_log_prob >= 0) {
            fprintf(stderr, "\nWARNING: sum_log_prob non negative\n");
            break;
        }

        if (callback != 0) {
            callback("sum_log_prob", (double) sum_log_prob);
        }

        // end iterations per -a parameter?
        if (val_auto > 0 && fabsl(change) <= abs_log_val_auto) {
            break;
        }

        sum_log_prob_prev = sum_log_prob;
    }
    fprintf(stderr, "\n");

    const double measure_elapsed_sec = measure_time_now_sec() - measure_start_sec;

    hmm_refinement_destroy();

    report_results();

    int res = hmm_save(hmm, model_filename);
    if (res) {
        fprintf(stderr, "error saving model \"%s\": %d\n", model_filename, res);
    }

    hmm_destroy(hmm);

    printf("training took %s\n", measure_time_show_elapsed(measure_elapsed_sec));
    return 0;
}
