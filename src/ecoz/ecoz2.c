#include "ecoz2.h"
#include "lpc.h"

#include <string.h>

const char *ecoz2_version() {
    return "ECOZ 2";
}

static char ecoz2_foo_result[256];

const char *ecoz2_foo(const char *name) {
    const char *prefix = "Hi ";
    strcpy(ecoz2_foo_result, prefix);
    strncat(ecoz2_foo_result, name, sizeof(ecoz2_foo_result) - 10);
    return ecoz2_foo_result;
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
