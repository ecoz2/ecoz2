/* hmm_gen.c -- ECOZ System
 */

#include "hmm.h"
#include "distr.h"

void hmm_genQ(Hmm *hmm, int *Q, int T) {
    Q[0] = dis_event(hmm->pi, hmm->N);

    for (int i = 1; i < T; i++) {
        Q[i] = dis_event(hmm->A[Q[i - 1]], hmm->N);
    }
}

void hmm_genO(Hmm *hmm, int *Q, int T, Symbol *O) {
    for (int i = 0; i < T; i++) {
        O[i] = dis_event(hmm->B[Q[i]], hmm->M);
    }
}
