/* hmm_adjustb.c -- ECOZ System
 */

#include "utl.h"
#include "hmm.h"
#include <math.h>
#include <stdlib.h>
#include <assert.h>

// to verify stochastic requirement
#define VERIFY 0

/*
 * if ε > 0, then each B[j][k] is adjusted
 * such that it must be >= ε.
 */
prob_t hmm_epsilon = 1.e-5;

void hmm_adjustB(Hmm *hmm, const char* logMsg) {
    if (hmm_epsilon <= (prob_t) 0.) {
        return;  // no restriction
    }

    prob_t **B = hmm->B;

    if (VERIFY) {
        for (int j = 0; j < hmm->N; j++) {
            prob_t sum = 0;
            for (int k = 0; k < hmm->M; k++) {
                if (B[j][k] < 0) {
                    printf(RED("hmm_adjustB: PRE ERROR: Σ B[%d][%d] = %e < 0 %s\n\n"),
                            j, k, B[j][k], logMsg);
                    exit(1);
                }
                sum += B[j][k];
            }
            if (fabsl(sum - 1) > 1e-10) {
                printf(RED("hmm_adjustB: PRE ERROR: Σ B[%d] = %e != 1 %s\n\n"),
                        j, sum, logMsg);
                //exit(1);
            }
        }
    }

    for (int j = 0; j < hmm->N; j++) {
        prob_t total_added = 0, total_extra = 0;
        for (int k = 0; k < hmm->M; k++) {
            if (B[j][k] < hmm_epsilon) {
                total_added += hmm_epsilon - B[j][k];
                B[j][k] = hmm_epsilon;
            }
            else if (B[j][k] > hmm_epsilon) {
                total_extra += B[j][k] - hmm_epsilon;
            }
        }
        if (total_added) {
            for (int k = 0; k < hmm->M; k++) {
                if (B[j][k] > hmm_epsilon) {
                    prob_t extra = B[j][k] - hmm_epsilon;
                    prob_t fraction = extra / total_extra;
                    B[j][k] -= total_added * fraction;
                }
            }

            if (VERIFY) {
                prob_t sum = 0;
                for (int k = 0; k < hmm->M; k++) {
                    if (B[j][k] < hmm_epsilon) {
                        printf(RED("hmm_adjustB: POST ERROR: Σ B[%d][%d] = %e < ε = %e %s\n\n"),
                                j, k, B[j][k], hmm_epsilon, logMsg);
                        exit(1);
                    }
                    sum += B[j][k];
                }
                if (fabsl(sum - 1) > 1e-10) {
                    printf(RED("hmm_adjustB: POST ERROR: Σ B[%d] = %e != 1 %s\n\n"),
                            j, sum, logMsg);
                    exit(1);
                }
            }
        }
    }
}
