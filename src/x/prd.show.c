/* prd.show.c -- ECOZ System
 */

#include "utl.h"
#include "lpc.h"

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

static void usage() {
    printf("\
\n\
ECOZ System\n\
\n\
	prd.show - Show predictors\n\
\n\
	prd.show <predictor> ...\n\
\n"
    );
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage();
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        Predictor *predictor = prd_load(argv[i]);
        if (!predictor) {
            printf(": error loading %s.\n", argv[i]);
            continue;
        }

        printf("%s:\n", argv[i]);
        prd_show(predictor);
        prd_destroy(predictor);
    }

    return 0;
}
