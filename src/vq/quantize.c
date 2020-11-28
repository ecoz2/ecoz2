/* quantize.c
 */

#include "vq.h"
#include "lpc.h"


sample_t quantize(sample_t *raas, int num_raas, Predictor *prd, Symbol *seq) {
    const int P = prd->P;
    const int T = prd->T;

    //printf(":: quantize: P=%d T=%d  seq=%p\n", P, T, seq);
    sample_t ddprm = 0;
    for (int t = 0; t < T; t++) {
        sample_t *rxg = prd->vectors[t];
        sample_t ddmin = SAMPLE_MAX;
        int i_min = 0;
        sample_t *raa = raas;
        for (int i = 0; i < num_raas; i++, raa += (1 + P)) {
            sample_t dd = distortion(rxg, raa, P);
            if (dd < ddmin) {
                ddmin = dd;
                i_min = i;
            }
        }
        ddprm += ddmin - 1;
        if (seq) {
            *seq++ = (Symbol) i_min;
        }
    }
    ddprm /= T;
    //printf(":: quantize done: distortion=%f\n", ddprm);
    return ddprm;
}
