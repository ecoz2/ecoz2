/* hmm_genQopt.c -- ECOZ System
 * maximum likelihood state sequence
 */

#include "hmm.h"
#include "utl.h"
#include <stdlib.h>
#include <math.h>

void hmm_precompute_logA(Hmm *hmm, prob_t **logA) {
    const int N = hmm->N;
    prob_t **A = hmm->A;

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            logA[i][j] = logl(A[i][j]);
        }
    }
}

void hmm_precompute_logB(Hmm *hmm, Symbol *O, int T, prob_t **logB) {
    const int N = hmm->N;
    prob_t **B = hmm->B;

    for (int j = 0; j < N; ++j) {
        for (int t = 0; t < T; ++t) {
            const int k = O[t];
            logB[j][k] = logl(B[j][k]);
        }
    }
}

prob_t hmm_genQopt(Hmm *hmm, Symbol *O, int T, int *Qopt) {
    const int N = hmm->N;
    const int M = hmm->M;

    prob_t **logA = (prob_t **) new_matrix(N, N, sizeof(prob_t));
    hmm_precompute_logA(hmm, logA);

    prob_t **logB = (prob_t **) new_matrix(N, M, sizeof(prob_t));
    hmm_precompute_logB(hmm, O, T, logB);

    prob_t **phi = (prob_t **) new_matrix(T, N, sizeof(prob_t));
    int **psi = (int **) new_matrix(T, N, sizeof(int));

    prob_t log_prob = hmm_genQopt_with_mem(hmm, O, T, Qopt,
            logA, logB,
            phi, psi);

    del_matrix(psi);
    del_matrix(phi);
    del_matrix(logB);
    del_matrix(logA);

    return log_prob;
}

prob_t hmm_genQopt_with_mem(Hmm *hmm, Symbol *O, int T, int *Qopt,
                            prob_t **logA, prob_t **logB,
                            prob_t **phi, int **psi) {
    const int N = hmm->N;
    prob_t *pi = hmm->pi;
//    prob_t **A = hmm->A;
//    prob_t **B = hmm->B;

    // psi calculation as in Rabiner (32-35)
    // phi calculation as in Rabiner (104-105)

    for (int i = 0; i < N; i++) {
//        phi[0][i] = logl(pi[i]) + logl(B[i][O[0]]);
        phi[0][i] = logl(pi[i]) + logB[i][O[0]];
        psi[0][i] = 0;
    }

    int argMax = 0;

    for (int t = 1; t < T; t++) {
        for (int j = 0; j < N; j++) {
            prob_t max_tj = 0.;
            for (int i = 0; i < N; i++) {
//                prob_t val = phi[t - 1][i] + logl(A[i][j]);
                prob_t val = phi[t - 1][i] + logA[i][j];
                if (i == 0 || max_tj < val) {
                    max_tj = val;
                    argMax = i;
                }
            }
//            phi[t][j] = max_tj + logl(B[j][O[t]]);
            phi[t][j] = max_tj + logB[j][O[t]];
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

    if (Qopt) {
        // optimal sequence:
        Qopt[T - 1] = argMax;
        for (int t = T - 2; t >= 0; t--) {
            Qopt[t] = psi[t + 1][Qopt[t + 1]];
        }
    }

    return log_prob;
}
