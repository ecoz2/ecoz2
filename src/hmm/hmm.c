/* hmm.c -- ECOZ System
 */

#include "hmm.h"
#include "distr.h"
#include "utl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


Hmm *hmm_create(const char *className, int N, int M) {
    Hmm *hmm = (Hmm *) malloc(sizeof(Hmm));
    if (!hmm) return 0;

    strcpy(hmm->className, className);
    hmm->N = N;
    hmm->M = M;

    hmm->pi = (prob_t *) new_vector(N, sizeof(prob_t));
    hmm->A = (prob_t **) new_matrix(N, N, sizeof(prob_t));
    hmm->B = (prob_t **) new_matrix(N, M, sizeof(prob_t));

    return hmm;
}

void hmm_destroy(Hmm *hmm) {
    del_vector(hmm->pi);
    del_matrix(hmm->A);
    del_matrix(hmm->B);
    free(hmm);
}

void hmm_init(Hmm *hmm, int mode) {
    switch (mode) {
        case HMM_RANDOM:
            dis_inicAle(hmm->pi, hmm->N);
            for (int i = 0; i < hmm->N; i++) {
                dis_inicAle(hmm->A[i], hmm->N);
                dis_inicAle(hmm->B[i], hmm->M);
            }
            break;

        case HMM_UNIFORM:
            dis_inicUni(hmm->pi, hmm->N);
            for (int i = 0; i < hmm->N; i++) {
                dis_inicUni(hmm->A[i], hmm->N);
                dis_inicUni(hmm->B[i], hmm->M);
            }
            break;

        case HMM_CASCADE2:
        case HMM_CASCADE3:
            /* pi: 1; A: delta=mode; B: random */

            dis_inicDelta(hmm->pi, hmm->N, 0, 1);

            for (int i = 0; i < hmm->N; i++) {
                dis_inicDelta(hmm->A[i], hmm->N, i, mode);
                dis_inicAle(hmm->B[i], hmm->M);
            }
            break;
    }
}

void hmm_show_model(Hmm *hmm, char *fto) {
    printf("className: '%s'  N=%d  M=%d\n", hmm->className, hmm->N, hmm->M);

    printf("\npi = ");
    prob_t sum = 0;
    for (int i = 0; i < hmm->N; i++) {
        prob_t p = hmm->pi[i];
        printf(fto, p);
        sum += p;
    }
    printf(" Σ = ");
    printf(fto, sum);
    printf("\n");
    if (fabsl(sum - 1) > 1e-10) {
        printf(RED("***ERROR** Σ pi = %Le != 1\n\n"), sum);
    }

    printf("\nA =  ");
    for (int i = 0; i < hmm->N; i++) {
        prob_t sum = 0;
        for (int j = 0; j < hmm->N; j++) {
            prob_t p = hmm->A[i][j];
            printf(fto, p);
            sum += p;
        }
        printf(" Σ = ");
        printf(fto, sum);
        printf("\n");
        if (fabsl(sum - 1) > 1e-10) {
            printf(RED("***ERROR** Σ A[%d] = %Le != 1\n\n"), i, sum);
        }
    }

    printf("\nB =  ");
    for (int i = 0; i < hmm->N; i++) {
        prob_t sum = 0;
        for (int j = 0; j < hmm->M; j++) {
            prob_t p = hmm->B[i][j];
            printf(fto, p);
            sum += p;
        }
        printf(" Σ = ");
        printf(fto, sum);
        printf("\n");
        if (fabsl(sum - 1) > 1e-10) {
            printf(RED("***ERROR** Σ B[%d] = %Le != 1\n\n"), i, sum);
        }
    }
}
