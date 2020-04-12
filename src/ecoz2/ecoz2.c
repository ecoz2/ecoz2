#include "ecoz2.h"
#include "lpc.h"
#include "vq.h"
#include "hmm.h"

#include <string.h>
#include <stdlib.h>  // sranddev, srand

const char *ecoz2_version() {
    return "0.2.0";
}

// for wrapper testing purposes
static char ecoz2_hi_result[256];
const char *ecoz2_hi(const char *name) {
    const char *prefix = "Hi ";
    strcpy(ecoz2_hi_result, prefix);
    strncat(ecoz2_hi_result, name, sizeof(ecoz2_hi_result) - 10);
    return ecoz2_hi_result;
}
int ecoz2_baz() {
    return 142857;
}

void ecoz2_set_random_seed(int seed) {
    if (seed < 0) {
        sranddev();
    }
    else {
        srand((unsigned) seed);
    }
}

int ecoz2_lpc_signals(
        int P,
        int windowLengthMs,
        int offsetLengthMs,
        int minpc,
        float split,
        char *sgn_filenames[],
        int num_signals
        ) {

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

int ecoz2_prd_show_file(
        char *filename,
        int show_reflections,
        int from,
        int to
        ) {

    return prd_show_file(
            filename,
            show_reflections,
            from,
            to
    );
}

int ecoz2_vq_learn(
        int prediction_order,
        sample_t epsilon,
        const char *codebook_class_name,
        const char *predictor_filenames[],
        int num_predictors
        ) {

    return vq_learn(
            prediction_order,
            epsilon,
            codebook_class_name,
            predictor_filenames,
            num_predictors
    );
}

int ecoz2_vq_quantize(
        const char *nom_raas,
        const char *predictor_filenames[],
        int num_predictors
        ) {

    return vq_quantize(
            nom_raas,
            predictor_filenames,
            num_predictors
    );
}

int ecoz2_vq_show(
        char *codebook_filename,
        int from,
        int to
        ) {

    return vq_show(
            codebook_filename,
            from,
            to
    );
}

int ecoz2_vq_classify(
        char **cb_filenames,
        int num_codebooks,
        char **prd_filenames,
        int num_predictors,
        int show_ranked
        ) {

    return vq_classify(
            cb_filenames,
            num_codebooks,
            prd_filenames,
            num_predictors,
            show_ranked
    );
}

int ecoz2_hmm_learn(
        int N,
        int model_type,
        const char* sequence_filenames[],
        int num_sequences,
        double hmm_epsilon,
        double val_auto,
        int max_iterations,
        hmm_learn_callback_t callback
        ) {

    return hmm_learn(
            N,
            model_type,
            sequence_filenames,
            num_sequences,
            hmm_epsilon,
            val_auto,
            max_iterations,
            callback
    );
}

int ecoz2_hmm_classify(
        char **model_names,
        int num_model_names,
        char **seq_filenames,
        int num_seq_filenames,
        int show_ranked
        ) {

    return hmm_classify(
            model_names,
            num_model_names,
            seq_filenames,
            num_seq_filenames,
            show_ranked
    );
}

int ecoz2_hmm_show(
        char *filename,
        char *format
        ) {

    return hmm_show(
            filename,
            format
    );
}

int ecoz2_seq_show_files(
        int with_prob,
        int gen_Qopt,
        int no_sequence,
        char* hmm_filename,
        char* seq_filenames[],
        int num_seq_filenames
        ) {

    return seq_show_files(
            with_prob,
            gen_Qopt,
            no_sequence,
            hmm_filename,
            seq_filenames,
            num_seq_filenames
    );
}
