/* vq.learn.c -- ECOZ System
 * Codebook generation according to Juang et al (1982).
 */

#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#include "lpc.h"
#include "vq.h"
#include "utl.h"


#define MAX_CODEBOOK_SIZE 2048

// prediction order (required parameter)
static int P = 0;

// to examine convergence:
static sample_t eps = .05;

// factors to grow codebook:
static const sample_t pert0 = 0.99f;
static const sample_t pert1 = 1.01f;


// Codebook:

static sample_t *codebook;       // codebook entries as raas
static sample_t *reflections;    // codebook entries as reflection vectors
static int num_raas;          // current number of entries
static sample_t *cells;          // classification on training vectors

static int cardd[MAX_CODEBOOK_SIZE];   // cardinalities

static sample_t discel[MAX_CODEBOOK_SIZE];    // distortions per cell

static char prefix[2048] = "";    // to name report and codebooks
static char cb_filename[2048];
static char codebook_className[2048];

static long tot_vecs;

static void allocate_codebook() {
    const long maxCbSizeInBytes = MAX_CODEBOOK_SIZE * (1 + P) * sizeof(sample_t);

    if (0 == (codebook = (sample_t *) malloc(maxCbSizeInBytes))
        || 0 == (cells = (sample_t *) malloc(maxCbSizeInBytes))
        || 0 == (reflections = (sample_t *) malloc(maxCbSizeInBytes))
            ) {
        printf("not enough memory for codebook\n");
        exit(1);
    }

    sample_t *raa = codebook;
    sample_t *refl = reflections;
    for (int i = 0; i < MAX_CODEBOOK_SIZE * (1 + P); i++, raa++, refl++) {
        *raa = *refl = (sample_t) 0.;
    }
}

static void release_codebook() {
    free(reflections);
    free(cells);
    free(codebook);
}

/*
 initial codebook corresponding to 2 reflection vectors [_, ±0.5, 0, ...].
 note: first entry (0) of reflection vector is ignored.
*/
static void initial_codebook() {
    num_raas = 2;

    sample_t *refl0 = reflections + 1;
    sample_t *refl1 = reflections + (1 + P) + 1;
    *refl0++ = -(*refl1++ = (sample_t) .5);

    // zero in all other entries:
    for (int i = 1; i < P; i++) {
        *refl0++ = *refl1++ = (sample_t) 0.;
    }

    // get the raas associated with these two reflection vectors:
    reflections_to_raas(reflections, codebook, num_raas, P);
}

static void init_cells() {
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

// adds autocorrelation rx to i-th cell and
// accumulates the distortion associated to such cell
static void add_to_cell(int i, sample_t *rx, sample_t ddmin) {
    sample_t *cell = cells + (1 + P) * i;
    for (int n = 0; n < (1 + P); n++, cell++, rx++) {
        *cell += *rx;
    }
    cardd[i]++;
    discel[i] += ddmin - 1;
}

static void review_cells(void) {
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

/**
  * Updates each cell's centroid in the form of reflection
  * coefficients by solving the LPC equations corresponding to
  * the average autocorrelation.
  */
static void calculate_reflections() {
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

/**
 * Grows the codebook by perturbing each reflector with the pert1 and pert0 factors.
 */
static void grow_codebook(int P) {
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
}

static void learn(sample_t **allVectors,
                  sample_t eps,
                  vq_learn_callback_t callback
                  ) {

    // to maintain and report codeword and minimum distortion for
    // each training vector against each trained codebook size:
    CodewordAndMinDist *minDists = (CodewordAndMinDist *) calloc(tot_vecs, sizeof(CodewordAndMinDist));

    int pass = 0;

    sprintf(cb_filename, "%s_M_%04d.cbook", prefix, num_raas);
    printf("%s\n", cb_filename);

    sample_t DDprv = SAMPLE_MAX;
    for (;;) {
        printf("(%d)", pass);
        fflush(stdout);

        init_cells();

        sample_t DD = 0;

        for (int v = 0; v < tot_vecs; v++) {
            sample_t *rxg = allVectors[v];
            sample_t ddmin = SAMPLE_MAX;
            int raa_min = -1;

            sample_t *raa = codebook;
            for (int i = 0; i < num_raas; i++, raa += (1 + P)) {
                sample_t dd = distortion(rxg, raa, P);
                if (dd < ddmin) {
                    ddmin = dd;
                    raa_min = i;
                }
            }
            minDists[v].codeword = raa_min;
            minDists[v].minDist = ddmin - 1;
            DD += ddmin - 1;
            add_to_cell(raa_min, rxg, ddmin);
        }

        const sample_t avgDistortion = DD / tot_vecs;

        printf("\tDP=%g\tDDprv=%g\tDD=%g\t(DDprv-DD)/DD=%g\r",
               avgDistortion, DDprv, DD, ((DDprv - DD) / DD));
        fflush(stdout);

        review_cells();

        calculate_reflections();

        if (pass > 0 && ((DDprv - DD) / DD) < eps) {
            // codebook saved with reflections
            cb_save(codebook_className, reflections, num_raas, P, cb_filename);

            sample_t sigma = calculateSigma(codebook, cells, num_raas, P, avgDistortion);
            sample_t inertia = calculateInertia(allVectors,  tot_vecs, codebook, num_raas, P);

            if (callback != 0) {
                callback(num_raas, avgDistortion, sigma, inertia);
            }

            report_cbook(cb_filename, pass + 1, avgDistortion, sigma, inertia, num_raas, cardd, discel,
                         minDists);

            printf("\n");

            if (num_raas >= MAX_CODEBOOK_SIZE) {
                break;
            }

            pass = 0;
            grow_codebook(P);
            sprintf(cb_filename, "%s_M_%04d.cbook", prefix, num_raas);
            printf("%s\n", cb_filename);
        }
        else {
            pass++;
        }

        DDprv = DD;
    }
    free(minDists);
}

int vq_learn(int prediction_order,
             sample_t epsilon,
             const char *codebook_class_name,
             const char *predictor_filenames[],
             int num_predictors,
             vq_learn_callback_t callback
        ) {

    P = prediction_order;
    eps = epsilon;
    strcpy(codebook_className, codebook_class_name);

    printf("\nCodebook generation:\n\n");
    printf("P=%d eps=%g  class='%s'\n\n", P, eps, codebook_className);

    const char *dir_codebooks = "data/codebooks";
    static char class_dir[2048];
    sprintf(class_dir, "%s/%s", dir_codebooks, codebook_className);
    mk_dirs(class_dir);
    sprintf(prefix, "%s/eps_%g", class_dir, eps);

    allocate_codebook();
    initial_codebook();

    // load predictors
    Predictor **predictors = (Predictor **) calloc(num_predictors, sizeof(Predictor*));
    tot_vecs = 0;
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

    learn(allVectors, eps, callback);

    free(allVectors);

    for (int i = 0; i < num_predictors; i++) {
        prd_destroy(predictors[i]);
    }
    free(predictors);

    close_report();

    release_codebook();
    return 0;
}
