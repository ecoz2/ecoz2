/* log[P(O | λ)]
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

static const int initialT = 1024;
static const int incrementT = 512;

void release_HmmProb_members(HmmProb *hmmprob) {
    if (hmmprob->T != 0) {
        del_vector(hmmprob->c);
        del_matrix(hmmprob->alphaH);
        del_matrix(hmmprob->alpha2);
        del_matrix(hmmprob->alpha);
    }
    hmmprob->alpha = hmmprob->alpha2 = hmmprob->alphaH = 0;
    hmmprob->T = 0;
}

static void allocate_HmmProb_members(HmmProb *hmmprob, int T) {
    Hmm *hmm = hmmprob->hmm;

    hmmprob->T = T;
    hmmprob->alpha = (prob_t **) new_matrix(T, hmm->N, sizeof(prob_t));
    hmmprob->alpha2 = (prob_t **) new_matrix(T, hmm->N, sizeof(prob_t));
    hmmprob->alphaH = (prob_t **) new_matrix(T, hmm->N, sizeof(prob_t));

    hmmprob->c = (prob_t *) new_vector(T, sizeof(prob_t));

    if (!hmmprob->alpha || !hmmprob->alpha2 || !hmmprob->alphaH) {
        fprintf(stderr, "not enough memory for hmmprob_create\n");
        exit(1);
    }
}

static void reallocate_HmmProb_members_if_needed(HmmProb *hmmprob, int T) {
    if (hmmprob->T < T) {
        release_HmmProb_members(hmmprob);
        int newT = hmmprob->T;
        while (newT < T) {
            newT += incrementT;
        }
        fprintf(stderr, "reallocating hmmprob (hmmprob->T=%d, requested T=%d, newT=%d)\n",
                hmmprob->T, T, newT);
        allocate_HmmProb_members(hmmprob, newT);
    }
}

HmmProb *hmmprob_create(Hmm *hmm) {
    HmmProb *hmmprob = (HmmProb *) malloc(sizeof(HmmProb));
    if (!hmmprob) {
        fprintf(stderr, "not enough memory for hmmprob_create\n");
        exit(1);
        return 0;
    }
    hmmprob->hmm = hmm;

    allocate_HmmProb_members(hmmprob, initialT);
    return hmmprob;
}

void hmmprob_destroy(HmmProb *hmmprob) {
    release_HmmProb_members(hmmprob);
    free(hmmprob);
}

prob_t hmmprob_log_prob(HmmProb *hmmprob, Symbol *O, int T) {
    reallocate_HmmProb_members_if_needed(hmmprob, T);

    Hmm *hmm = hmmprob->hmm;
    const int N = hmm->N;
    prob_t *pi = hmm->pi;
    prob_t **A = hmm->A;
    prob_t **B = hmm->B;

    prob_t **alpha  = hmmprob->alpha;
    prob_t **alpha2 = hmmprob->alpha2;
    prob_t **alphaH = hmmprob->alphaH;
    prob_t *c = hmmprob->c;

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

    return log_prob;
}
