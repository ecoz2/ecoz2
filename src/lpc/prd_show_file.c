/* prd_show_file.c -- ECOZ System
 */

#include "utl.h"
#include "lpc.h"

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>


int prd_show_file(
        char *filename,
        int show_reflections,
        int from,
        int to
        ) {

    Predictor *predictor = prd_load(filename);
    if (!predictor) {
        printf(": error loading %s.\n", filename);
        return 2;
    }

    if (from < 0) {
        from = 1;
    }
    if (to < 0 || to > predictor->P) {
        to = predictor->P;
    }

    printf("# %s:\n", filename);
    printf("# className='%s', T=%d, P=%d\n",
           predictor->className, predictor->T, predictor->P);

    const char *comma = "";
    for (int k = from; k <= to; k++) {
        printf("%s%c%d", comma, show_reflections ? 'k' : 'r', k);
        comma = ",";
    }
    printf("\n");

    sample_t refl[predictor->P + 1];
    sample_t pred[predictor->P + 1];
    sample_t errPred;

    for (int t = 0; t < predictor->T; t++) {
        sample_t *coeffs;

        sample_t *vector = predictor->vectors[t];
        if (show_reflections) {
            lpca_r(predictor->P, vector, refl, pred, &errPred);
            coeffs = refl;
        }
        else {
            coeffs = vector;
        }

        comma = "";
        for (int p = from; p <= to; p++) {
            printf("%s%g", comma, coeffs[p]);
            comma = ",";
        }
        printf("\n");
    }

    prd_destroy(predictor);

    return 0;
}
