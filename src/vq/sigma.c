/* sigma.c
 */

#include "lpc.h"
#include "vq.h"

/**
  * Computes `dpc / avgDistortion`, the sigma ratio for a given codebook,
  * where `dpc` is the average inter-cell distortion
  * and `avgDistortion` is the average intra-cell distortion
  */
sample_t calculateSigma(sample_t *codebook, sample_t *cells, int codebookSize, int P, sample_t avgDistortion) {
    sample_t dpc = 0;
    sample_t *raa = codebook;
    for (int i = 0; i < codebookSize; i++, raa += (1 + P)) {
        sample_t dis_i = 0.f;
        sample_t *rcc = cells;
        for (int j = 0; j < codebookSize; j++, rcc += (1 + P)) {
            if (i != j) {
                dis_i += distortion(rcc, raa, P) - 1;
            }
        }
        dpc += dis_i;
    }
    dpc /= codebookSize * (codebookSize - 1);
    sample_t sigma = dpc / avgDistortion;

    return sigma;
}
