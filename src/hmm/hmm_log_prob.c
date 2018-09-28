/* hmm_log_prob.c -- ECOZ System
 *
 * hmm_log_prob: log[P(O | λ)]
 *
 * Uses scaling factors as explained in "Some Mathematics for HMM," Dawei Shen (2008)
 * http://courses.media.mit.edu/2010fall/mas622j/ProblemSets/ps4/tutorial.pdf
 */

#include "hmm.h"
#include "utl.h"

#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>

static const prob_t one = (prob_t) 1.;

// TODO(performance): preparations with single memory allocation prior to
// repeated calls to hmm_log_prob

prob_t hmm_log_prob(Hmm *hmm, Symbol *O, int T) {
    const int N = hmm->N;
    prob_t *pi = hmm->pi;
    prob_t **A = hmm->A;
    prob_t **B = hmm->B;

    prob_t **alpha = (prob_t **) new_matrix(T, hmm->N, sizeof(prob_t));
    prob_t **alpha2 = (prob_t **) new_matrix(T, hmm->N, sizeof(prob_t));
    prob_t **alphaH = (prob_t **) new_matrix(T, hmm->N, sizeof(prob_t));
    prob_t *c = (prob_t *) new_vector(T, sizeof(prob_t));

    if (!alpha || !alpha2 || !alphaH || !c) {
        fprintf(stderr, "not enough memory to hmm_log_prob\n");
        exit(1);
        return 0;
    }

    prob_t sumAlpha2 = 0.;
    for (int i = 0; i < N; i++) {
        prob_t val = pi[i] * B[i][O[0]];
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
            sumAlpha2_ti *= B[i][O[t]];

            alpha2[t][i] = sumAlpha2_ti;
            sumAlpha2_t += sumAlpha2_ti;
        }

        c[t] = one / sumAlpha2_t;
        for (int i = 0; i < N; i++) {
            alphaH[t][i] = c[t] * alpha2[t][i];
        }
    }

    prob_t log_prob = 0.;
    for (int t = 0; t < T; t++) {
        log_prob -= logl(c[t]);
    }

    del_vector(c);
    del_matrix(alphaH);
    del_matrix(alpha2);
    del_matrix(alpha);

    return log_prob;
}
