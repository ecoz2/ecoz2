/* inertia.c
 */

#include "lpc.h"
#include "vq.h"

/**
  * Computes the "inertia" (within-cluster sum-of-squares) of a given codebook.
  */
sample_t calculateInertia(sample_t **allVectors, long tot_vecs,
                          sample_t *codebook, int num_raas, int P
                         ) {
    sample_t inertia = 0;

    for (int v = 0; v < tot_vecs; v++) {
        sample_t *rxg = allVectors[v];
        sample_t ddmin = SAMPLE_MAX;

        sample_t *raa = codebook;
        for (int i = 0; i < num_raas; i++, raa += (1 + P)) {
            sample_t dd = distortion(rxg, raa, P);
            dd = dd * dd;
            if (dd < ddmin) {
                ddmin = dd;
            }
        }
        inertia += ddmin;
    }

    return inertia;
}
