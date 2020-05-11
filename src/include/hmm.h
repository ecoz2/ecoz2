/* hmm.h -- ECOZ System
 */

#ifndef __ECOZ_HMM_H
#define __ECOZ_HMM_H

#include "ecoz2.h"
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

#define MAX_MODELS 256

int hmm_learn(
        int N,
        int model_type,
        const char* sequence_filenames[],
        unsigned num_sequences,
        double hmm_epsilon,
        double val_auto,
        int max_iterations,
        int user_par,
        hmm_learn_callback_t callback
        );

int hmm_classify(
        char **model_names,
        unsigned num_model_names,
        char **seq_filenames,
        unsigned num_seq_filenames,
        int show_ranked_
        );

int hmm_show(char *filename, char *format);

int seq_show_files(
        int with_prob,
        int gen_Qopt,
        int no_sequence,
        char* hmm_filename,
        char* seq_filenames[],
        int num_seq_filenames
      );

Hmm *hmm_create(const char *className, int N, int M);

Hmm *hmm_load(char *filename);

int hmm_save(Hmm *, char *filename);

/// Adjust B according to epsilon restriction:
///  if hmm_epsilon > 0, then each B[j][k] is adjusted
///  such that it is >= hmm_epsilon.  B[j] is of course re-normalized.
void hmm_adjust_B_epsilon(Hmm *hmm, const char* logMsg);

void hmm_destroy(Hmm *);

/// Initial B estimation based on symbol frequencies.
/// max_T > 0 can be given if already known.
void hmm_estimateB(Hmm *hmm, Symbol **O, int *Ts, int num_seqs, int max_T);

/**
 * Model initialization according to given mode.
 */
void hmm_init(Hmm *, int mode);

/**
 * Initialization of A matrix row according to given mode.
 */
void hmm_init_A_row(prob_t *row, int N, int mode, int colForCascade);

/**
 * Helper for multiple hmm_log_prob calculations wrt same model.
 */
typedef struct {
    Hmm *hmm;

    int T;
    prob_t **alpha;
    prob_t **alpha2;
    prob_t **alphaH;
    prob_t *c;
} HmmProb;

HmmProb *hmmprob_create(Hmm *hmm);

void hmmprob_destroy(HmmProb *);

prob_t hmmprob_log_prob(HmmProb *hmmprob, Symbol *O, int T);

prob_t hmm_log_prob(Hmm *hmm, Symbol *O, int T);


/// precompute log(A[i][j])
void hmm_precompute_logA(Hmm *hmm, prob_t **logA);

/// precompute log(B[j][O[t]])
void hmm_precompute_logB(Hmm *hmm, Symbol *O, int T, prob_t **logB);

/// Generates maximum likelihood state sequence.
/// Returns associated probability.
/// @param Qopt  Where the optimal state sequence is stored.
///              Could be null when only the probability is of interest
///
prob_t hmm_genQopt(Hmm *, Symbol *O, int T, int *Qopt);

/// As with hmm_genQopt but with precomputed logs and work memory
/// provided by caller
prob_t hmm_genQopt_with_mem(Hmm *hmm, Symbol *O, int T, int *Qopt,
                            prob_t **logA, prob_t **logB,
                            prob_t **phi, int **psi);


void hmm_show_model(Hmm *, char *fmt);

/**
 * Generates a state sequence
 */
void hmm_genQ(Hmm *, int *Q, int T);

/**
 * Generates an observation sequence.
 */
void hmm_genO(Hmm *, int *Q, int T, Symbol *O);

#endif
