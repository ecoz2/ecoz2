/* hmm_refinement.c -- ECOZ System
 *
 * HMM model parameter re-estimation based on multiple input sequences.
 *
 * Modified from my original implementation to incorporate scaling factors
 * as explained in "Some Mathematics for HMM," Dawei Shen (2008)
 * http://courses.media.mit.edu/2010fall/mas622j/ProblemSets/ps4/tutorial.pdf
 */

#include "hmm.h"
#include "utl.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <assert.h>

static Hmm *hmm;
static Symbol **seqs;
static int *T;
static int num_cads;

static prob_t **alpha;
static prob_t **alpha2;  // alpha-dot-dot
static prob_t **alphaH;  // alpha-hat

static prob_t *c;        // c(t) scaling factor

static prob_t **beta;
static prob_t **beta2;   // beta-dot-dot
static prob_t **betaH;   // beta-hat

static prob_t **numA;
static prob_t *denA;
static prob_t **numB;
static prob_t *denB;

static const prob_t one = (prob_t) 1.;

//#define obs(t) (assert(O[(t)] < M), O[t])
#define obs(t) O[t]
//#define obs(t) (O[(t)] < M ? O[(t)] : (printf("OOPS! O[%d]=%d >= M=%d T=%d\n", (t), O[(t)], M, T), seq_show(O, T), assert((t) < M), O[t]))

int hmm_refinement_prepare(Hmm *_lmd, Symbol **_cads, int *_T, int _num_cads) {
    hmm = _lmd;
    seqs = _cads;
    T = _T;
    num_cads = _num_cads;

    numA = (prob_t **) new_matrix(hmm->N, hmm->N, sizeof(prob_t));
    numB = (prob_t **) new_matrix(hmm->N, hmm->M, sizeof(prob_t));
    denA = (prob_t *) new_vector(hmm->N, sizeof(prob_t));
    denB = (prob_t *) new_vector(hmm->N, sizeof(prob_t));

    // use max T for the following memory allocations
    int max_T = 0;
    for (int r = 0; r < num_cads; r++) {
        if (max_T < T[r]) max_T = T[r];
    }

    alpha = (prob_t **) new_matrix(max_T, hmm->N, sizeof(prob_t));
    alpha2 = (prob_t **) new_matrix(max_T, hmm->N, sizeof(prob_t));
    alphaH = (prob_t **) new_matrix(max_T, hmm->N, sizeof(prob_t));
    c = (prob_t *) new_vector(max_T, sizeof(prob_t));

    beta = (prob_t **) new_matrix(max_T, hmm->N, sizeof(prob_t));
    beta2 = (prob_t **) new_matrix(max_T, hmm->N, sizeof(prob_t));
    betaH = (prob_t **) new_matrix(max_T, hmm->N, sizeof(prob_t));

    if (0 == numA || 0 == denA
        || 0 == numB || 0 == denB
        || 0 == alpha || 0 == alpha2 || 0 == alphaH
        || 0 == c
        || 0 == beta || 0 == beta2 || 0 == betaH
            ) {
        hmm_refinement_destroy();
        return 1;
    }

    return 0;
}

void hmm_refinement_destroy() {
    if (0 != betaH) {
        del_matrix(betaH);
        betaH = 0;
    }
    if (0 != beta2) {
        del_matrix(beta2);
        beta2 = 0;
    }
    if (0 != beta) {
        del_matrix(beta);
        beta = 0;
    }
    if (0 != c) {
        del_vector(c);
        c = 0;
    }
    if (0 != alphaH) {
        del_matrix(alphaH);
        alphaH = 0;
    }
    if (0 != alpha2) {
        del_matrix(alpha2);
        alpha2 = 0;
    }
    if (0 != alpha) {
        del_matrix(alpha);
        alpha = 0;
    }
    if (0 != denB) {
        del_vector(denB);
        denB = 0;
    }
    if (0 != denA) {
        del_vector(denA);
        denA = 0;
    }
    if (0 != numB) {
        del_matrix(numB);
        numB = 0;
    }
    if (0 != numA) {
        del_matrix(numA);
        numA = 0;
    }
}

static void init_counters() {
    const int N = hmm->N;
    const int M = hmm->M;

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            numA[i][j] = 0.;
        }
        denA[i] = 0.;
    }

    for (int j = 0; j < N; j++) {
        for (int k = 0; k < M; k++) {
            numB[j][k] = 0.;
        }
        denB[j] = 0.;
    }
}

static void gen_alpha_beta(Symbol *O, int T) {
    const int N = hmm->N;
    prob_t *pi = hmm->pi;
    prob_t **A = hmm->A;
    prob_t **B = hmm->B;

    prob_t sumAlpha2 = 0.;
    for (int i = 0; i < N; i++) {
        prob_t val = pi[i] * B[i][obs(0)];
        alpha[0][i] = val;
        alpha2[0][i] = val;
        sumAlpha2 += val;
    }

    c[0] = one / sumAlpha2;
    for (int i = 0; i < N; i++) {
        alphaH[0][i] = c[0] * alpha2[0][i];
    }

    for (int t = 1; t < T; t++) {
        prob_t sumAlpha2_t = 0.;

        for (int i = 0; i < N; i++) {
            prob_t sumAlpha2_ti = 0.;
            for (int j = 0; j < N; j++) {
                sumAlpha2_ti += alphaH[t - 1][j] * A[j][i];
            }
            sumAlpha2_ti *= B[i][obs(t)];

            alpha2[t][i] = sumAlpha2_ti;
            sumAlpha2_t += sumAlpha2_ti;
        }

        c[t] = one / sumAlpha2_t;
        for (int i = 0; i < N; i++) {
            alphaH[t][i] = c[t] * alpha2[t][i];
        }

        if (0) {
            prob_t sum = 0;
            for (int i = 0; i < N; i++) {
                sum += alphaH[t][i];
            }
            if (fabsl(sum - 1) > 1e-10) {
                printf(RED("gen_alpha_beta: ERROR: Σ alphaH[%d] = %Le != 1\n\n"),
                       t, sum);
            }
        }
    }

    for (int i = 0; i < N; i++) {
        beta2[T - 1][i] = 1.;
        betaH[T - 1][i] = c[T - 1];   // c[T-1] * beta2[T-1][i]
    }

    for (int t = T - 2; t >= 0; t--) {
        for (int i = 0; i < N; i++) {
            prob_t sumBeta2_ti = 0.;
            for (int j = 0; j < N; j++) {
                sumBeta2_ti += A[i][j] * B[j][obs(t + 1)] * betaH[t + 1][j];
            }
            beta2[t][i] = sumBeta2_ti;
            betaH[t][i] = c[t] * sumBeta2_ti;
        }
    }
}

static void gen_numA_denA(Symbol *O, int T) {
    const int N = hmm->N;
    prob_t **A = hmm->A;
    prob_t **B = hmm->B;

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            prob_t num_sum_t = 0.;
            for (int t = 0; t < T - 1; t++) {
                num_sum_t += alphaH[t][i] * A[i][j] * B[j][obs(t + 1)] * betaH[t + 1][j];
            }
            numA[i][j] += num_sum_t;
        }

        prob_t den_sum_t = 0.;
        for (int t = 0; t < T - 1; t++) {
            den_sum_t += alphaH[t][i] * betaH[t][i] / c[t];
        }
        denA[i] += den_sum_t;
    }
}

static void gen_numB_denB(Symbol *O, int T) {
    const int N = hmm->N;
    const int M = hmm->M;

    for (int j = 0; j < N; j++) {
        for (int k = 0; k < M; k++) {
            prob_t num_sum_t = 0.;
            for (int t = 0; t < T; t++) {
                if (obs(t) == k) {
                    num_sum_t += alphaH[t][j] * betaH[t][j] / c[t];
                }
            }
            numB[j][k] += num_sum_t;
        }

        prob_t den_sum_t = 0.;
        for (int t = 0; t < T; t++) {
            den_sum_t += alphaH[t][j] * betaH[t][j] / c[t];
        }
        denB[j] += den_sum_t;
    }
}

static void refine_A() {
    const int N = hmm->N;
    prob_t **A = hmm->A;

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = numA[i][j] / denA[i];
        }
    }
}

static void refine_B() {
    const int N = hmm->N;
    const int M = hmm->M;
    prob_t **B = hmm->B;

    for (int j = 0; j < N; j++) {
        for (int k = 0; k < M; k++) {
            B[j][k] = numB[j][k] / denB[j];
        }
    }
    hmm_adjustB(hmm, "refine_B");
}

int hmm_refinement_step() {
    init_counters();

    // process each sequence:
    for (int r = 0; r < num_cads; r++) {
        gen_alpha_beta(seqs[r], T[r]);

        gen_numA_denA(seqs[r], T[r]);
        gen_numB_denB(seqs[r], T[r]);
    }

    // refine parameters:
    refine_A();
    refine_B();

    return 0;
}
