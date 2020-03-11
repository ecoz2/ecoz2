/* hmm.h -- ECOZ System
 */

#ifndef __ECOZ_HMM_H
#define __ECOZ_HMM_H

#include "utl.h"
#include "symbol.h"
#include "prob_t.h"

/**
 * HMM representation
 */
typedef struct {
    char className[MAX_CLASS_NAME_LEN];
    int N;         // number of states
    int M;         // number of symbols
    prob_t *pi;    // initial state distribution
    prob_t **A;    // state transition distributions
    prob_t **B;    // symbol emitting distributions
} Hmm;


/**
 * Modes to initialize model parameters.
 */
enum hmm_modes_init {
    HMM_RANDOM = 0,   // pi, A, B: no restriction; non-zero probability values
    HMM_UNIFORM = 1,  // pi, A, B: uniform distributions
    HMM_CASCADE2 = 2, // left-to-right, cascade-2 (same state and next state)
    HMM_CASCADE3 = 3  // left-to-right, cascade-3 (same state and next two states)
};


// value of epsilon restriction for B matrix (default, 1.e-5):
extern prob_t hmm_epsilon;

// maximum number of training sequences
#define MAX_SEQS 4096

int hmm_learn(
        int N,
        int model_type,
        const char* sequence_filenames[],
        int num_sequences,
        double hmm_epsilon,
        double val_auto,
        int max_iterations
        );


Hmm *hmm_create(const char *className, int N, int M);

Hmm *hmm_load(char *filename);

int hmm_save(Hmm *, char *filename);

/**
 * Adjust B according to epsilon restriction
 */
void hmm_adjustB(Hmm *, const char* logMsg);

void hmm_destroy(Hmm *);

/**
 * Initial B estimation based on symbol frequencies.
 */
void hmm_estimateB(Hmm *hmm, Symbol **O, int *T, int num_cads);

/**
 * Model initialization according to given mode.
 */
void hmm_init(Hmm *, int mode);

/**
 * HMM parameter refinement with scaling.
 */
int hmm_refinement_prepare(Hmm *hmm, Symbol **seqs, int *T, int num_cads);

int hmm_refinement_step();

void hmm_refinement_destroy();

prob_t hmm_log_prob(Hmm *hmm, Symbol *O, int T);

/**
 * Generates maximum likelihood state sequence
 */
prob_t hmm_genQopt(Hmm *, Symbol *O, int T, int *Qopt);

void hmm_show(Hmm *, char *fmt);

/**
 * Generates a state sequence
 */
void hmm_genQ(Hmm *, int *Q, int T);

/**
 * Generates an observation sequence.
 */
void hmm_genO(Hmm *, int *Q, int T, Symbol *O);

#endif
