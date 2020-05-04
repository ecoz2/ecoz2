/* hmm_genQopt.c -- ECOZ System
 * maximum likelihood state sequence
 */

#include "hmm.h"
#include "utl.h"
#include <stdlib.h>
#include <math.h>

prob_t hmm_genQopt(Hmm *hmm, Symbol *O, int T, int *Qopt) {
    const int N = hmm->N;

    prob_t **phi = (prob_t **) new_matrix(T, N, sizeof(prob_t));
    int **psi = (int **) new_matrix(T, N, sizeof(int));

    prob_t log_prob = hmm_genQopt_with_mem(hmm, O, T, Qopt, phi, psi);

    del_matrix(psi);
    del_matrix(phi);

    return log_prob;
}

prob_t hmm_genQopt_with_mem(Hmm *hmm, Symbol *O, int T, int *Qopt,
                            prob_t **phi, int **psi) {
    const int N = hmm->N;
    prob_t *pi = hmm->pi;
    prob_t **A = hmm->A;
    prob_t **B = hmm->B;

    for (int i = 0; i < N; i++) {
        phi[0][i] = logl(pi[i]) + logl(B[i][O[0]]);
        psi[0][i] = 0;
    }

    int argMax = 0;

    for (int t = 1; t < T; t++) {
        for (int j = 0; j < N; j++) {
            prob_t max_tj = 0.;
            for (int i = 0; i < N; i++) {
                prob_t val = phi[t - 1][i] + logl(A[i][j]);
                if (i == 0 || max_tj < val) {
                    max_tj = val;
                    argMax = i;
                }
            }
            phi[t][j] = max_tj + logl(B[j][O[t]]);
            psi[t][j] = argMax;
        }
    }

    prob_t log_prob = 0.;
    for (int i = 0; i < N; i++) {
        prob_t val = phi[T - 1][i];
        if (i == 0 || log_prob < val) {
            log_prob = val;
            argMax = i;
        }
    }

    // optimal sequence:
    Qopt[T - 1] = argMax;
    for (int t = T - 2; t >= 0; t--) {
        Qopt[t] = psi[t + 1][Qopt[t + 1]];
    }

    return log_prob;
}
