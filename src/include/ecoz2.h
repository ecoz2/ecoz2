/* ecoz2.h -- ECOZ System
 */

#ifndef __ECOZ2_H
#define __ECOZ2_H

#ifdef __cplusplus
extern "C" {
#endif

const char *ecoz2_version();

/// Calls `sranddev()` if seed < 0;
/// otherwise, calls `srand((unsigned) seed)`.
void ecoz2_set_random_seed(int seed);

int ecoz2_lpc_signals(
        int P,
        int windowLengthMs,
        int offsetLengthMs,
        int minpc,
        float split,
        char *sgn_filenames[],
        int num_signals
        );

int ecoz2_prd_show_file(
        char *filename,
        int show_reflections,
        int from,
        int to
        );

typedef void (*vq_learn_callback_t)(char*, double);

int ecoz2_vq_learn(
        int prediction_order,
        double epsilon,
        const char *codebook_class_name,
        const char *predictor_filenames[],
        int num_predictors,
        vq_learn_callback_t callback
        );

int ecoz2_vq_quantize(
        const char *nom_raas,
        const char *predictor_filenames[],
        int num_predictors
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
        int num_sequences,
        double hmm_epsilon,
        double val_auto,
        int max_iterations,
        hmm_learn_callback_t callback
        );

int ecoz2_hmm_classify(
        char **model_names,
        int num_model_names,
        char **seq_filenames,
        int num_seq_filenames,
        int show_ranked
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
