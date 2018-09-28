/* hmm_estimateB.c -- ECOZ System
 */

#include "hmm.h"
#include "utl.h"

#include <stdlib.h>

static const prob_t one = (prob_t) 1.;

void hmm_estimateB(Hmm *hmm, Symbol **seqs, int *T, int num_cads) {
    const int N = hmm->N;
    const int M = hmm->M;
    prob_t **B = hmm->B;

    // counters:

    // num[j][k] number of times symbol k emitted from state j:
    int **num = (int **) new_matrix(N, M, sizeof(int));

    // def[j] number of times any symbol emitted from state j:
    int *den = (int *) new_vector(N, sizeof(int));

    int max_T = 0;
    for (int r = 0; r < num_cads; r++) {
        if (max_T < T[r]) max_T = T[r];
    }
    int *Qopt = (int *) new_vector(max_T, sizeof(int));

    for (int r = 0; r < num_cads; r++) {
        // get optimal sequence:
        hmm_genQopt(hmm, seqs[r], T[r], Qopt);

        // update counters:
        for (int t = 0; t < T[r]; t++) {
            num[Qopt[t]][seqs[r][t]]++;
            den[Qopt[t]]++;
        }
    }
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

    del_matrix(num);
    del_vector(den);

    // now apply epsilon-restriction:
    hmm_adjustB(hmm, "hmm_estimateB");
}
