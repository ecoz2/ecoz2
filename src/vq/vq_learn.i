#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#include "lpc.h"
#include "vq.h"
#include "utl.h"

#define MAX_CODEBOOK_SIZE_IN_VALUES (MAX_CODEBOOK_SIZE * (1 + MAX_PREDICTION_ORDER))

// codebook entries as raas:
static sample_t codebook[MAX_CODEBOOK_SIZE_IN_VALUES];

// codebook entries as reflection vectors:
static sample_t reflections[MAX_CODEBOOK_SIZE_IN_VALUES];

// classification on training vectors:
static sample_t cells[MAX_CODEBOOK_SIZE_IN_VALUES];

static inline void init_codebook_and_reflections(int P) {
    sample_t *raa = codebook;
    sample_t *refl = reflections;
    const int num_values = MAX_CODEBOOK_SIZE * (1 + P);
    for (int i = 0; i < num_values; i++, raa++, refl++) {
        *raa = *refl = (sample_t) 0.;
    }
}

/// initial codebook corresponding to 2 reflection vectors [_, ±0.5, 0, ...].
/// note: first entry (0) of reflection vector is ignored.
static inline int initial_codebook(int P) {
    sample_t *refl0 = reflections + 1;
    sample_t *refl1 = reflections + (1 + P) + 1;
    *refl0++ = -(*refl1++ = (sample_t) .5);

    // zero all other entries:
    for (int i = 1; i < P; i++) {
        *refl0++ = *refl1++ = (sample_t) 0.;
    }

    // get the raas associated with these two reflection vectors:
    reflections_to_raas(reflections, codebook, 2, P);
    return 2;
}

static inline void init_cells(int P, sample_t *cells, int num_raas, int *cardd, sample_t *discel) {
    // set (1+P)*num_raas values to zero:
    sample_t *cel = cells;
    for (int i = 0; i < (1 + P) * num_raas; i++, cel++) {
        *cel = (sample_t) 0.;
    }

    for (int i = 0; i < num_raas; i++) {
        cardd[i] = 0;
        discel[i] = 0.f;
    }
}

/// adds autocorrelation rx to i-th cell, updates cardinality and
/// accumulates the distortion associated to such cell
static inline void add_to_cell(int P, sample_t *rx, sample_t ddmin, int *cardd, sample_t *discel, int i) {
    sample_t *cell = cells + (1 + P) * i;
    for (int n = 0; n < (1 + P); n++, cell++, rx++) {
        *cell += *rx;
    }
    cardd[i]++;
    discel[i] += ddmin - 1;
}

static inline void review_cells(int num_raas, int *cardd) {
    int numEmpty = 0;
    for (int i = 0; i < num_raas; i++) {
        if (0 == cardd[i]) {
            numEmpty++;
        }
    }
    if (numEmpty > 0) {
        printf("\nWARN: review_cells: %d empty cell(s) for codebook size %d)\n",
               numEmpty, num_raas);
    }
}


/// Updates each cell's centroid in the form of reflection
/// coefficients by solving the LPC equations corresponding to
/// the average autocorrelation.
static void calculate_reflections(int P, int *cardd, int num_raas) {
    sample_t errPred, pred[1 + P];

    sample_t *raa = codebook;
    sample_t *cel = cells;
    sample_t *refl = reflections;

    for (int i = 0; i < num_raas; i++, raa += (1 + P), cel += (1 + P), refl += (1 + P)) {
        if (cardd[i] > 0) {
            lpca_r(P, cel, refl, pred, &errPred);

            // pred[] has the predictor coefficients; now get the corresponding
            // autocorrelation as entry in the codebook:
            for (int n = 0; n <= P; n++) {
                sample_t sum = 0;
                for (int k = 0; k <= P - n; k++) {
                    sum += pred[k] * pred[k + n];
                }
                raa[n] = sum;
            }

            // normalize accumulated autocorrelation sequence by gain
            // in this cell for the sigma ratio calculation:
            if (errPred != 0.) {
                for (int k = 0; k <= P; k++) {
                    cel[k] /= errPred;
                }
            }
        }
    }
}


/// Grows the codebook by perturbing each reflector with the pert1 and pert0 factors.
static int grow_codebook(int num_raas, int P) {
    // factors to grow codebook:
    const sample_t pert0 = 0.99f;
    const sample_t pert1 = 1.01f;

    sample_t *rex = reflections + (1 + P) * num_raas - 1;    // start with last coefficient
    sample_t *rex1 = rex + (1 + P) * num_raas;            // new coeff with pert1
    sample_t *rex0 = rex1 - (1 + P);                    // new coeff with pert0

    for (int i = 0; i < num_raas; i++, rex1 -= (1 + P), rex0 -= (1 + P)) {
        for (int n = 0; n < (1 + P); n++, rex--, rex1--, rex0--) {
            *rex1 = *rex * pert1;
            *rex0 = *rex * pert0;
        }
    }
    num_raas *= 2;

    reflections_to_raas(reflections, codebook, num_raas, P);

    return num_raas;
}
