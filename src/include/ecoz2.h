/* ecoz2.h -- ECOZ System
 */

#ifndef __ECOZ2_H
#define __ECOZ2_H

#ifdef __cplusplus
extern "C" {
#endif

const char *ecoz2_version(void);

/// Calls `srand(rand())` if seed < 0;
/// otherwise, calls `srand((unsigned) seed)`.
/// Returns the seed actually used.
unsigned long ecoz2_set_random_seed(long seed);

/// Returns the seed actually used in the last call to ecoz2_set_random_seed.
/// Returns -1 if no such call has been made. Otherwise the return value
/// can be cast to `unsigned`.
long ecoz2_get_random_seed_used(void);

int ecoz2_lpc_signals(
        int P,
        int windowLengthMs,
        int offsetLengthMs,
        int minpc,
        float split,
        char *sgn_filenames[],
        int num_signals,
        float mintrpt,   // min time in secs to report processing time per signal
        int verbose
        );

/**
 * Computes the predictor and reflection coefficients using the autocorrelation method.
 * Implementation based on fortran version by Parsons (1987).
 */
int ecoz2_lpca(
        double *x,        // the input signal
        int N,            // number of samples in input signal
        int P,            // prediction order
        double *r,        // out: autocorrelation
        double *rc,       // out: reflections
        double *a,        // out: predictor
        double *pe        // out: prediction error
        );

int ecoz2_prd_show_file(
        char *filename,
        int show_reflections,
        int from,
        int to
        );

typedef void (*vq_learn_callback_t)(
        void* callback_target,
        int M, double avgDistortion, double sigma, double inertia
        );

/// Clustering starting from codebook size 2
int ecoz2_vq_learn(
        int prediction_order,
        double epsilon,
        const char *codebook_class_name,
        const char *predictor_filenames[],
        int num_predictors,
        void* callback_target,
        vq_learn_callback_t callback
        );

/// Clustering starting from next size according to given codebook
int ecoz2_vq_learn_using_base_codebook(
        char *base_codebook,
        double epsilon,
        const char *predictor_filenames[],
        int num_predictors,
        void* callback_target,
        vq_learn_callback_t callback
        );

int ecoz2_vq_quantize(
        const char *nom_raas,
        const char *predictor_filenames[],
        int num_predictors,
        int show_filenames
        );

int ecoz2_vq_show(
        char *codebook_filename,
        int from,
        int to
        );

int ecoz2_vq_classify(
        char **cb_filenames,
        int num_codebooks,
        char **prd_filenames,
        int num_predictors,
        int show_ranked
        );

// double to facilitate rust wrapper
typedef void (*hmm_learn_callback_t)(char*, double);

int ecoz2_hmm_learn(
        int N,
        int model_type,
        const char* sequence_filenames[],
        unsigned num_sequences,
        double hmm_epsilon,
        double val_auto,
        int max_iterations,
        int use_par,
        hmm_learn_callback_t callback
        );

int ecoz2_hmm_classify(
        char **model_names,
        unsigned num_model_names,
        char **seq_filenames,
        unsigned num_seq_filenames,
        int show_ranked,
        const char *classification_filename
        );

int ecoz2_hmm_show(
        char *filename,
        char *format
        );

int ecoz2_seq_show_files(
        int with_prob,
        int gen_Qopt,
        int no_sequence,
        char* hmm_filename,
        char* seq_filenames[],
        int num_seq_filenames
        );

#ifdef __cplusplus
}
#endif

#endif
