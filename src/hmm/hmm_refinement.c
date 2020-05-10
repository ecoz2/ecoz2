/* hmm_refinement.c -- ECOZ System
 *
 * HMM model parameter re-estimation based on multiple input sequences.
 *
 * Modified from my original implementation to incorporate scaling factors
 * as explained in [^rab89], [^stamp], [^shen].
 */


#include "hmm.h"
#include "utl.h"
#include "distr.h"

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

static prob_t *c;        // c(t) scaling factor

static prob_t **beta;

static prob_t **gamma1;   // gamma1[t][i]
static prob_t ***gamma2;  // gamma2[t][i][j]

static prob_t **numA;
static prob_t *denA;
static prob_t **numB;
static prob_t *denB;

static const prob_t zero = (prob_t) 0.;
static const prob_t one = (prob_t) 1.;

//#define obs(t) (assert(O[(t)] < M), O[t])
//#define obs(t) O[t]
#define obs(t) (O[(t)] < M ? O[(t)] : (printf("OOPS! O[%d]=%d >= M=%d T=%d\n", (t), O[(t)], M, T), seq_show(O, T), assert((t) < M), O[t]))

static void hmm_refinement_destroy(int max_T);

static int hmm_refinement_prepare(Hmm *_lmd, Symbol **_cads, int *_T, int _num_cads) {
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
    c = (prob_t *) new_vector(max_T, sizeof(prob_t));

    beta = (prob_t **) new_matrix(max_T, hmm->N, sizeof(prob_t));

    gamma1 = (prob_t **) new_matrix(max_T, hmm->N, sizeof(prob_t));
    gamma2 = (prob_t ***) new_matrix3(max_T, hmm->N, hmm->N);

    if (0 == numA
        || 0 == denA
        || 0 == numB
        || 0 == denB
        || 0 == alpha
        || 0 == c
        || 0 == beta
        || 0 == gamma1
        || 0 == gamma2
            ) {
        hmm_refinement_destroy(max_T);
        return 1;
    }

    return 0;
}

static void hmm_refinement_destroy(int max_T) {
    if (0 != gamma1) {
        del_matrix(gamma1);
        gamma1 = 0;
    }
    if (0 != gamma2) {
        del_matrix3(gamma2, max_T, hmm->N);
        gamma2 = 0;
    }
    if (0 != beta) {
        del_matrix(beta);
        beta = 0;
    }
    if (0 != c) {
        del_vector(c);
        c = 0;
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

static void init_counters(void) {
    const int N = hmm->N;
    const int M = hmm->M;

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            numA[i][j] = zero;
        }
        denA[i] = zero;
    }

    for (int j = 0; j < N; j++) {
        for (int k = 0; k < M; k++) {
            numB[j][k] = zero;
        }
        denB[j] = zero;
    }
}

static inline void generate_gammas(Symbol *O, int T) {
    const int N = hmm->N;
    const int M = hmm->M;  // for obs macro
    prob_t *pi = hmm->pi;
    prob_t **A = hmm->A;
    prob_t **B = hmm->B;

    /// The alpha-pass

    // compute alpha[0][i]
    c[0] = zero;
    for (int i = 0; i < N; i++) {
        prob_t val = pi[i] * B[i][obs(0)];
        alpha[0][i] = val;
        c[0] += val;
    }

    // scale alpha[0][i]
    c[0] = one / c[0];
    for (int i = 0; i < N; i++) {
        alpha[0][i] *= c[0];
    }

    // compute alpha[t][i]
    for (int t = 1; t < T; t++) {
        c[t] = zero;

        for (int i = 0; i < N; i++) {
            alpha[t][i] = zero;
            for (int j = 0; j < N; j++) {
                alpha[t][i] += alpha[t - 1][j] * A[j][i];
            }
            alpha[t][i] *= B[i][obs(t)];
            c[t] += alpha[t][i];
        }

        // scale alpha[t][i]
        c[t] = one / c[t];
        for (int i = 0; i < N; i++) {
            alpha[t][i] *= c[t];
        }

        if ((0)) {
            prob_t sum = 0;
            for (int i = 0; i < N; i++) {
                sum += alpha[t][i];
            }
            if (fabsl(sum - 1) > 1e-10) {
                printf(RED("generate_gammas: ERROR: Σ alpha[%d] = %Le != 1\n\n"),
                       t, sum);
            }
        }
    }

    /// The beta-pass

    // let beta[T - 1][i] = 1, scaled by c[T - 1]
    for (int i = 0; i < N; i++) {
        beta[T - 1][i] = c[T - 1];
    }

    for (int t = T - 2; t >= 0; t--) {
        for (int i = 0; i < N; i++) {
            beta[t][i] = zero;
            for (int j = 0; j < N; j++) {
                beta[t][i] += A[i][j] * B[j][obs(t + 1)] * beta[t + 1][j];
            }
            // scale beta[t][i] with same scale factor as alpha[t][i]
            beta[t][i] *= c[t];
        }
    }

    /// Compute gamma2[t][i][j] and gamma1[t][i]

    // No need to normalize gamma2[t][i][j] since using scaled alpha and beta
    for (int t = 0; t < T - 1; ++t) {
        for (int i = 0; i < N; ++i) {
            gamma1[t][i] = zero;
            for (int j = 0; j < N; ++j) {
                gamma2[t][i][j] = alpha[t][i] * A[i][j] * B[j][obs(t + 1)] * beta[t + 1][j];
                gamma1[t][i] += gamma2[t][i][j];
            }
        }
    }

    // Special case for gamma1[T - 1][i] (as above, no need to normalize)
    for (int i = 0; i < N; ++i) {
        gamma1[T - 1][i] = alpha[T - 1][i];
    }
}

/// accumulate numA and denA according to sequence just processed
static inline void accumulate_numA_denA(int T) {
    const int N = hmm->N;

    for (int i = 0; i < N; ++i) {
        prob_t denom = zero;
        for (int t = 0; t < T - 1; ++t) {
            denom += gamma1[t][i];
        }
        denA[i] += denom;

        for (int j = 0; j < N; ++j) {
            prob_t numer = zero;
            for (int t = 0; t < T - 1; ++t) {
                numer += gamma2[t][i][j];
            }
            numA[i][j] += numer;
        }
    }
}

/// accumulate numB and denB according to this sequence
static inline void accumulate_numB_denB(Symbol *O, int T) {
    const int N = hmm->N;
    const int M = hmm->M;

    for (int i = 0; i < N; ++i) {
        prob_t denom = zero;
        for (int t = 0; t < T; ++t) {
            denom += gamma1[t][i];
        }
        denB[i] += denom;

        for (int j = 0; j < M; ++j) {
            prob_t numer = zero;
            for (int t = 0; t < T; ++t) {
                if (obs(t) == j) {
                    numer += gamma1[t][i];
                }
            }
            numB[i][j] += numer;
        }
    }
}

static inline void reestimate_A(void) {
    const int N = hmm->N;
    prob_t **A = hmm->A;

    int num_zero_dens = 0;
    for (int i = 0; i < N; i++) {
        if (denA[i] != zero) {
            for (int j = 0; j < N; j++) {
                A[i][j] = numA[i][j] / denA[i];
            }
        }
        else {
            ++num_zero_dens;
            hmm_init_A_row(A[i], N, model_type, i);
            // TODO or perhaps just leave A[i] as it is?
        }
    }
    if (num_zero_dens) {
        printf("%s(%d)", RED("!"), num_zero_dens);
        fflush(stdout);
    }
}

static inline void reestimate_B(void) {
    const int N = hmm->N;
    const int M = hmm->M;
    prob_t **B = hmm->B;

    int num_zero_dens = 0;
    for (int i = 0; i < N; ++i) {
        if (denB[i] != zero) {
            for (int j = 0; j < M; ++j) {
                B[i][j] = numB[i][j] / denB[i];
            }
        }
        else {
            ++num_zero_dens;
            dis_inicAle(B[i], M);
            // TODO or perhaps just leave B[i] as it is?
        }
    }
    if (num_zero_dens) {
        printf("%s(%d)", RED("¡"), num_zero_dens);
        fflush(stdout);
    }
    hmm_adjustB(hmm, "reestimate_B");
}

static int hmm_refinement_step(void) {
    //const double measure_start_sec = measure_time_now_sec();
    init_counters();

    // process each sequence:
    for (int r = 0; r < num_cads; r++) {
        generate_gammas(seqs[r], T[r]);

        accumulate_numA_denA(T[r]);
        accumulate_numB_denB(seqs[r], T[r]);
    }

    // refine parameters:
    reestimate_A();
    reestimate_B();

    //printf("hmm_refinement_step took %s\n", measure_time_now_sec() - measure_start_sec);

    return 0;
}
