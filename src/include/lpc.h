/* lpc.h -- ECOZ System
 * Linear Predictive Coding
 */

#ifndef __ECOZ_LPC_H
#define __ECOZ_LPC_H

#include "sgn.h"
#include "utl.h"

#include <float.h>

/**
 * Predictor representation
 */
typedef struct {
    char className[MAX_CLASS_NAME_LEN];
    sample_t **vectors;
    int T;
    int P;
} Predictor;


/**
 * Computes the predictor and reflection coefficients using the autocorrelation method.
 * Implementation based on fortran version by Parsons (1987).
 */
int lpca(sample_t *x,      // the input signal
         int N,            // number of samples in input signal
         int P,            // prediction order
         sample_t *r,      // out: autocorrelation
         sample_t *rc,     // out: reflections
         sample_t *a,      // out: predictor
         sample_t *pe      // out: prediction error
        );

/**
 * LPC analysis as done by `lpca` but with given autocorrelation as input.
 */
int lpca_r(int P,          // prediction order
           sample_t *r,    // autocorrelation
           sample_t *rc,   // out: reflections
           sample_t *a,    // out: predictor
           sample_t *pe    // out: prediction error
          );

/**
 * LPC analysis as done by `lpca` but with given reflection coefficients as input.
 */
int lpca_rc(int P,          // prediction order
            sample_t *rc,   // reflections
            sample_t *a     // out: predictor
           );

/**
 * Linear prediction analysis on a signal.
 *
 * @param P               Prediction order
 * @param windowLengthMs  Size of each frame in milliseconds
 * @param offsetLengthMs  Offset in milliseconds
 * @param sgn             The signal
 * @return                Resulting predictor
 */
Predictor *lpaOnSignal(int P, int windowLengthMs, int offsetLengthMs, Sgn *sgn);

/**
 * Gets the autocorrelation vectors (raas) corresponding to
 * the given reflection vectors.
 */
void reflections_to_raas(sample_t *reflections, sample_t *raas, int num_raas, int P);


Predictor *prd_create(int T, int P, const char *className);

Predictor *prd_load(char *nom_prd);

void prd_destroy(Predictor *prd);

int prd_save(Predictor *prd, char *nom_prd);

void prd_show(Predictor *predictor);

#endif
