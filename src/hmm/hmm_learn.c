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
#include <omp.h>

static int model_type = HMM_CASCADE3;

#include "hmm_refinement.c"


// for auto convergence
static prob_t val_auto = 0.3;
static prob_t log_val_auto;
static prob_t abs_log_val_auto;

// number of states and symbols
static int N = 5, M;

static int num_seqs;

static prob_t sum_log_prob;

static char model_className[MAX_CLASS_NAME_LEN];

static char model_filename[2048];

static int num_refinements;


static void _report_results(FILE *file, const char *prefix) {
    fprintf(file, "%s Model: %s   (seed=%ld)'\n", prefix,
            model_filename, ecoz2_get_random_seed_used());
    fprintf(file, "%s className: '%s'\n", prefix,
            model_className);
    fprintf(file, "%s N=%d M=%d type: %s\n", prefix,
            N, M,
           model_type == 0 ? "no restriction" :
           model_type == 1 ? "uniform distributions" :
           model_type == 2 ? "cascade-2" : "cascade-3");
    fprintf(file, "%s restriction: ", prefix);
    if ((prob_t) 0. == hmm_epsilon) fprintf(file, "No");
    else fprintf(file, "%Lg", (long double) hmm_epsilon);
    fprintf(file, "\n");
    fprintf(file, "%s     #sequences: %d\n",  prefix, num_seqs);
    fprintf(file, "%s     auto value: %Lg\n", prefix, (long double) val_auto);
    fprintf(file, "%s   #refinements: %d\n",  prefix, num_refinements);
    fprintf(file, "%s       Σ log(P): %Lg\n", prefix, (long double) sum_log_prob);
}

static void report_results(void) {
    printf("\n");
    _report_results(stdout, "    ");

    char rpt_filename[2048];
    strcpy(rpt_filename, model_filename);
    camext(rpt_filename, ".rpt");
    FILE *rpt_file = fopen(rpt_filename, "w");
    if (rpt_file) {
        _report_results(rpt_file, "");
        fclose(rpt_file);
    }
    else {
        fprintf(stderr, "ERROR creating report %s\n", rpt_filename);
    }
}

static inline prob_t get_sum_log_prob(Hmm *hmm, int num_seqs, Symbol **sequences, int *T, int use_par) {
    //const double measure_start_sec = measure_time_now_sec();
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
//            printf("{%d: prob=%Le} ", r, (long double) prob); fflush(stdout);
            sum_log_prob += prob;
        }
//        printf("\n");
    }

    //printf("get_sum_log_prob took %s\n", measure_time_show_elapsed(measure_time_now_sec() - measure_start_sec));
    return sum_log_prob;
}

static FILE *file_csv = 0;

void csv_prepare(char *csv_filename, int max_T) {
    if (0 == (file_csv = fopen(csv_filename, "w"))) {
        printf("error creating %s\n", csv_filename);
        return;
    }

    fprintf(file_csv, "# N=%d M=%d type=%d  #sequences = %d  max_T=%d  (seed=%ld)\n",
            N, M, model_type, num_seqs, max_T, ecoz2_get_random_seed_used());
    fprintf(file_csv, "%s,%s\n", "I", "Σ log(P)");
}

void csv_add_line(int i, prob_t sum_log_prob) {
    if (0 == file_csv) {
        return;
    }

    fprintf(file_csv, "%d,%Lg\n", i, (long double) sum_log_prob);
}

void csv_close(void) {
    if (file_csv) {
        fclose(file_csv);
        file_csv = 0;
    }
}

int hmm_learn(
        int N_,
        int model_type_,
        const char* sequence_filenames[],
        unsigned num_sequences,
        double hmm_epsilon_,
        double val_auto_,
        int max_iterations,
        int use_par,
        hmm_learn_callback_t callback
        ) {


    if (num_sequences > MAX_SEQS) {
        fprintf(stderr, "%d: too many sequences (max %d)\n", num_sequences, MAX_SEQS);
        return 1;
    }

    N = N_;
    model_type = model_type_;
    hmm_epsilon = (prob_t) hmm_epsilon_;
    val_auto = val_auto_;

    printf("\n--- hmm_learn ---  (use_par=%d)\n", use_par);
    printf("num_sequences = %d\n", num_sequences);
    printf("epsilon = %Lg\n", (long double) hmm_epsilon);

    printf("(sizeof(prob_t) = %lu)\n", sizeof(prob_t));


    // training sequences:
    Symbol *sequences[MAX_SEQS];

    // corresponding sequence lengths:
    int T[MAX_SEQS];

    char sequence_className[MAX_CLASS_NAME_LEN];

    // model to generate:
    Hmm *hmm;

    // load first training sequence:

    printf("  %3d: %s\n", 0, sequence_filenames[0]);
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
        sprintf(hmmDir, "data/hmms/N%d__M%d_t%d__a%Lg_I%d", N, M, model_type,
                (long double) val_auto, max_iterations);
    }
    else {
        sprintf(hmmDir, "data/hmms/N%d__M%d_t%d__a%Lg", N, M, model_type,
                (long double) val_auto);
    }
    mk_dirs(hmmDir);
    #pragma GCC diagnostic ignored "-Wformat-overflow"
    sprintf(model_filename, "%s/%s.hmm", hmmDir, model_className);

    num_seqs = 1;
    for (unsigned r = 1; r < num_sequences; ++r) {
        // to check all sequences come from same codebook size:
        int Mcmp;

        int show_filename = 0;
        if (num_sequences > 8) {
            const int part_size = 2;
            if (r == part_size) {
                printf("  ...\n");
            }
            else if (r < part_size || r > num_sequences - part_size - 1) {
                show_filename = 1;
            }
        }
        else show_filename = 1;
        if (show_filename) {
            printf("  %3d: %s\n", r, sequence_filenames[r]);
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
            if (strcmp(model_className, sequence_className)) {
                printf("WARNING: model '%s' trained with sequence '%s'\n",
                       model_className, sequence_className)
            }
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

    printf("\nN=%d M=%d type=%d  #sequences = %d  max_T=%d\n",
            N, M, model_type, num_seqs, max_T);
    printf("val_auto = %Lg   log=%Lg   max_iterations=%d\n",
           (long double) val_auto, (long double) log_val_auto,
           max_iterations);

    char csv_filename[2048];
    #pragma GCC diagnostic ignored "-Wformat-overflow"
    sprintf(csv_filename, "%s/%s.csv", hmmDir, model_className);
    csv_prepare(csv_filename, max_T);

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
    //printf("::::: sum_log_prob=%Le\n", (long double) sum_log_prob);
    csv_add_line(0, sum_log_prob);

    // do training:

    if (hmm_refinement_prepare(hmm, sequences, T, num_seqs)) {
        fprintf(stderr, "No memory for HMM refinement.\n");
        csv_close();
        return 2;
    }
    printf("refinement info prepared\n");

    char change_str[2014];

    num_refinements = 0;
    prob_t sum_log_prob_prev = sum_log_prob;
    while (max_iterations < 0 || num_refinements < max_iterations) {

        const double measure_ref_start_sec = measure_time_now_sec();

        if (hmm_refinement_step()) {
            fprintf(stderr, "refinement error\n");
            csv_close();
            return 2;
        }
        num_refinements++;
        printf(".");
        fflush(stderr);

        // probabilities post-refinement:
        sum_log_prob = get_sum_log_prob(hmm, num_seqs, sequences, T, use_par);
        //printf("::::: sum_log_prob=%Le\n", (long double) sum_log_prob);
        csv_add_line(num_refinements + 1, sum_log_prob);

        // measure and report refinement change:
        const prob_t change = sum_log_prob - sum_log_prob_prev;

        if (change < zero) {
            sprintf(change_str, RED("%+10.6Lg"), (long double) change);
        }
        else {
            sprintf(change_str, "%+10.6Lg", (long double) change);
        }
        printf(" %3d: Δ = %s  sum_log_prob = %+10.6Lg  prev = %+10.6Lg  '%s'  (%.3fs)\n",
                num_refinements,
                change_str,
                (long double) sum_log_prob,
                (long double) sum_log_prob_prev,
                model_className,
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
    printf("\n");

    csv_close();

    const double measure_elapsed_sec = measure_time_now_sec() - measure_start_sec;

    hmm_refinement_destroy(max_T);

    report_results();

    int res = hmm_save(hmm, model_filename);
    if (res) {
        fprintf(stderr, "error saving model \"%s\": %d\n", model_filename, res);
    }

    hmm_destroy(hmm);

    printf("=> training took %s     class=%s\n\n",
            measure_time_show_elapsed(measure_elapsed_sec), model_className);
    return 0;
}
