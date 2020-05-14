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

static inline void init_max_codebook_and_reflections(int P) {
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

/// Grows the codebook by perturbing each reflector with the pert1 and pert0 factors.
static int grow_codebook(int num_raas, int P) {
    if (num_raas == MAX_CODEBOOK_SIZE) {
        fprintf(stderr, "ERROR: maximum codebook size reached: %d", MAX_CODEBOOK_SIZE);
        exit(1);
    }

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

/// initial codebook from the given reflections
static inline int initial_codebook_from_base(sample_t *base_reflections, int base_num_vecs, int P) {
    long mem_size = (long) base_num_vecs * (1 + P) * sizeof(sample_t);
    memcpy(reflections, base_reflections, mem_size);

    // get the raas associated with these reflections:
    reflections_to_raas(reflections, codebook, base_num_vecs, P);
    return grow_codebook(base_num_vecs, P);
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
static inline void add_to_cell(int P, sample_t *cells, sample_t *rx, sample_t ddmin, int *cardd, sample_t *discel, int i) {
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
static void calculate_reflections(int P, sample_t *cells, int *cardd, int num_raas) {
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


#include "vq_learn_par.c"
#include "vq_learn_ser.c"

int vq_learn_(
        sample_t *base_reflections,
        int base_num_vecs,
        int prediction_order,
        sample_t epsilon,
        const char *codebook_class_name,
        const char *predictor_filenames[],
        int num_predictors,
        int use_par,
        void* callback_target,
        vq_learn_callback_t callback
        ) {

    const int P = prediction_order;
    const sample_t eps = epsilon;

    const char *dir_codebooks = "data/codebooks";
    static char class_dir[2048];
    sprintf(class_dir, "%s/%s", dir_codebooks, codebook_class_name);
    mk_dirs(class_dir);

    // to name report and codebooks
    char prefix[2048];
#pragma GCC diagnostic ignored "-Wformat-overflow"
    sprintf(prefix, "%s/eps_%g", class_dir, eps);

    init_max_codebook_and_reflections(P);

    int num_raas;
    if (base_reflections) {
        num_raas = initial_codebook_from_base(base_reflections, base_num_vecs, P);
    }
    else {
        num_raas = initial_codebook(P);
    }

    // load predictors
    Predictor **predictors = (Predictor **) calloc(num_predictors, sizeof(Predictor*));
    long tot_vecs = 0;
    for (int i = 0; i < num_predictors; i++) {
        const char *prdFilename = predictor_filenames[i];
        predictors[i] = prd_load(prdFilename);
        if (!predictors[i]) {
            fprintf(stderr, "error loading predictor %s\n", prdFilename);
            return 2;
        }
        tot_vecs += predictors[i]->T;
    }
    printf("%ld training vectors (ε=%g)\n", tot_vecs, eps);

    sample_t **allVectors = (sample_t **) calloc(tot_vecs, sizeof(sample_t *));
    int v = 0;
    for (int i = 0; i < num_predictors; i++) {
        for (int t = 0; t < predictors[i]->T; t++) {
            allVectors[v++] = predictors[i]->vectors[t];
        }
    }

    char nom_rpt[2048];
    sprintf(nom_rpt, "%s.rpt", prefix);
    prepare_report(nom_rpt, tot_vecs, eps);

    if (use_par) {
        learn_par(codebook_class_name,
                  P, prefix,
                  num_raas, tot_vecs, allVectors, eps, callback_target, callback);

    }
    else {
        printf("(using serialized impl)\n");
        learn_ser(codebook_class_name,
                  P, prefix,
                  num_raas, tot_vecs, allVectors, eps, callback_target, callback);
    }


    free(allVectors);

    for (int i = 0; i < num_predictors; i++) {
        prd_destroy(predictors[i]);
    }
    free(predictors);

    close_report();

    return 0;
}
