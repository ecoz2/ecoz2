/* hmm_estimateB.c -- ECOZ System
 */

#include "hmm.h"
#include "utl.h"

#include <stdlib.h>

static const prob_t one = (prob_t) 1.;

void hmm_estimateB(Hmm *hmm, Symbol **seqs, int *T, int num_cads, int max_T) {
    printf("estimating initial B matrix ...  (given max_T=%d)\n", max_T);
    const double measure_start_sec = measure_time_now_sec();

    const int N = hmm->N;
    const int M = hmm->M;
    prob_t **B = hmm->B;

    if (max_T <= 0) {
        max_T = 0;
        for (int r = 0; r < num_cads; ++r) {
            if (max_T < T[r]) max_T = T[r];
        }
        printf("(computed max_T=%d)\n", max_T);
    }

    // counters:

    // num[j][k] number of times symbol k emitted from state j:
    int **num = (int **) new_matrix(N, M, sizeof(int));

    // def[j] number of times any symbol emitted from state j:
    int *den = (int *) new_vector(N, sizeof(int));

    int *Qopt = (int *) new_vector(max_T, sizeof(int));

    prob_t **logA = (prob_t **) new_matrix(N, N, sizeof(prob_t));
    hmm_precompute_logA(hmm, logA);

    prob_t **logB = (prob_t **) new_matrix(N, M, sizeof(prob_t));

    prob_t **phi = (prob_t **) new_matrix(max_T, N, sizeof(prob_t));
    int **psi = (int **) new_matrix(max_T, N, sizeof(int));

    for (int r = 0; r < num_cads; r++) {
        hmm_precompute_logB(hmm, seqs[r], T[r], logB);

        // get optimal sequence:
        hmm_genQopt_with_mem(hmm, seqs[r], T[r], Qopt,
                logA, logB,
                phi, psi);

        // update counters:
        for (int t = 0; t < T[r]; t++) {
            const int state = Qopt[t];
            num[state][seqs[r][t]]++;
            den[state]++;
        }
    }
    del_matrix(psi);
    del_matrix(phi);
    del_matrix(logB);
    del_matrix(logA);

    del_vector(Qopt);

    // do required normalizations:
    for (int j = 0; j < N; j++) {
        // no emitted symbol from this state?
        if (den[j] == 0) {
            // set uniform distribution:
            for (int k = 0; k < M; k++) {
                B[j][k] = one / M;
            }
        }
        else {
            // set B[j] with normalized distribution:
            for (int k = 0; k < M; k++) {
                B[j][k] = ((prob_t) num[j][k]) / den[j];
            }
        }
    }

    del_vector(den);
    del_matrix(num);

    printf("initial B matrix took %s\n", measure_time_show_elapsed(measure_time_now_sec() - measure_start_sec));

    // now apply epsilon-restriction:
    hmm_adjustB(hmm, "hmm_estimateB");
}
