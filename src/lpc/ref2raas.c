/* ref2raas.c -- ECOZ System
 */

#include "lpc.h"

#include <float.h>

void reflections_to_raas(sample_t *reflections, sample_t *raas, int num_raas, int P) {
    sample_t pred[1 + P];

    sample_t *refl = reflections;
    sample_t *raa = raas;
    for (int i = 0; i < num_raas; i++, raa += (1 + P), refl += (1 + P)) {
        lpca_rc(P, refl, pred);

        // pred[] has the predictor coefficients; get the corresponding
        // autocorrelation as entry in the codebook:
        for (int n = 0; n <= P; n++) {
            sample_t sum = 0;
            for (int k = 0; k <= P - n; k++) {
                sum += pred[k] * pred[k + n];
            }
            raa[n] = sum;
        }
    }
}
