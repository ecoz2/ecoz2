/* hmm_adjustb.c -- ECOZ System
 */

#include "utl.h"
#include "hmm.h"
#include <math.h>
#include <stdlib.h>
#include <assert.h>

// to verify stochastic requirement
//#define VERIFY

prob_t hmm_epsilon = 1.e-5;

void hmm_adjust_B_epsilon(Hmm *hmm, const char* logMsg) {
    if (hmm_epsilon <= (prob_t) 0.) {
        return;  // no restriction
    }

    (void) logMsg; // avoid compile warning when VERIFY undefined.

    const int N = hmm->N;
    const int M = hmm->M;
    prob_t **B = hmm->B;

#ifdef VERIFY
    {
        for (int j = 0; j < N; j++) {
            prob_t *row = B[j];

            prob_t sum = 0;
            for (int k = 0; k < M; k++) {
                if (row[k] < 0) {
                    printf(RED("hmm_adjust_B_epsilon: PRE ERROR: Σ B[%d][%d] = %Le < 0 %s\n\n"),
                            j, k, row[k], logMsg);
                    exit(1);
                }
                sum += row[k];
            }
            if (fabsl(sum - 1) > 1e-10) {
                printf(RED("hmm_adjust_B_epsilon: PRE ERROR: Σ B[%d] = %Le != 1 %s\n\n"),
                        j, sum, logMsg);
                //exit(1);
            }
        }
    }
#endif

    for (int j = 0; j < N; j++) {
        prob_t *row = B[j];

        prob_t total_added = 0, total_extra = 0;
        for (int k = 0; k < M; k++) {
            if (row[k] < hmm_epsilon) {
                total_added += hmm_epsilon - row[k];
                row[k] = hmm_epsilon;
            }
            else if (row[k] > hmm_epsilon) {
                total_extra += row[k] - hmm_epsilon;
            }
        }
        if (total_added) {
            for (int k = 0; k < M; k++) {
                if (row[k] > hmm_epsilon) {
                    prob_t extra = row[k] - hmm_epsilon;
                    prob_t fraction = extra / total_extra;
                    row[k] -= total_added * fraction;
                }
            }

#ifdef VERIFY
            {
                prob_t sum = 0;
                for (int k = 0; k < M; k++) {
                    prob_t *row = B[j];

                    if (row[k] < hmm_epsilon) {
                        printf(RED("hmm_adjust_B_epsilon: POST ERROR: Σ B[%d][%d] = %Le < ε = %Le %s\n\n"),
                                j, k, row[k], hmm_epsilon, logMsg);
                        exit(1);
                    }
                    sum += row[k];
                }
                if (fabsl(sum - 1) > 1e-10) {
                    printf(RED("hmm_adjust_B_epsilon: POST ERROR: Σ B[%d] = %Le != 1 %s\n\n"),
                            j, sum, logMsg);
                    exit(1);
                }
            }
#endif
        }
    }
}
