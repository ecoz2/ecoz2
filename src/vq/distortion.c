/* distortion.c
 */

#include "vq.h"

sample_t distortion(sample_t *rx, sample_t *ra, int P) {
    sample_t term2 = (sample_t) 0.;
    sample_t term1 = *rx++ * *ra++;
    for (int n = 0; n < P; n++, rx++, ra++) {
        term2 += *rx * *ra;
    }
    return term1 + 2. * term2;
}
