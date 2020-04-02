#include "ecoz2.h"
#include "lpc.h"
#include "hmm.h"

#include <string.h>

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
