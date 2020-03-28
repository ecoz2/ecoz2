/* vq.classify.c -- ECOZ System
 */

#include "vq.h"
#include "utl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <float.h>
#include <assert.h>

#define MAX_MODELS    MAX_CB_MODELS

#define MAX_CLASSES   128

// loaded models
static Codebook *models[MAX_MODELS];


// row index for the totals:
#define    TOTAL        (MAX_CLASSES-1)

/* result[classId][0] is the number of tests for the given class.
 * result[classId][i], i > 0, number of correct tests with
 *                   i-th candidate (with 1 being the best)
 * result[TOTAL] will have the totals.
 */
static int result[MAX_CLASSES][MAX_MODELS + 1];

/* confusion matrix:
 * confusion[i][j] = number of times instances of class i
 *                   were recognized as of class j
 */
static int confusion[TOTAL][TOTAL];

static int num_models;

static int get_classId(const char *className) {
    for (int i = 0; i < num_models; i++) {
        if (0 == strcmp(className, models[i]->className)) {
            return i;
        }
    }
    return -1;
}

// show ranked for incorrect classifications?
static int show_ranked = 0;

static void init_results() {
    for (int classId = 0; classId <= TOTAL; classId++) {
        for (int i = 0; i <= num_models; i++) {
            result[classId][i] = 0;
        }
    }
    for (int i = 0; i < TOTAL; i++) {
        for (int j = 0; j < TOTAL; j++) {
            confusion[i][j] = 0;
        }
    }
}

static void report_results() {
    if (result[TOTAL][0] == 0)
        return;

    int margin = 0;
    for (int i = 0; i < TOTAL; i++) {
        if (result[i][0]) {
            int len = strlen(models[i]->className);
            if (margin < len) {
                margin = len;
            }
        }
    }
    margin += 2;

    printf("\n\n");
    printf("%*s ", margin, "");
    printf("Confusion matrix:\n");

    printf("%*s ", margin, "");

    printf("     ");
    for (int j = 0; j < TOTAL; j++) {
        if (result[j][0] > 0) {
            printf("%3d ", j);
        }
    }
    printf("    tests   errors\n");

    for (int i = 0; i < TOTAL; i++) {
        if (result[i][0] == 0) {
            continue;
        }

        printf("\n");
        printf("%*s ", margin, models[i]->className);
        printf("%3d  ", i);

        int num_errs = 0;   // in row
        for (int j = 0; j < TOTAL; j++) {
            if (result[j][0] > 0) {
                printf("%3d ", confusion[i][j]);
                if (i != j) {
                    num_errs += confusion[i][j];
                }
            }
        }
        printf("%8d%8d", result[i][0], num_errs);
    }

    printf("\n\n");
    printf("%*s ", margin, "");
    printf("class     accuracy    tests      candidate order\n");

    for (int classId = 0; classId <= TOTAL; classId++) {
        if (result[classId][0] == 0)
            continue;

        if (classId < TOTAL) {
            printf("%*s ", margin, models[classId]->className);
            printf("  %3d    ", classId);
        }
        else {
            printf("\n");
            printf("%*s ", margin, "");
            printf("  TOTAL  ");
        }

        printf("  %6.2f%%   %3d        ",
               (float) (100.0 * result[classId][1] / result[classId][0]),
               result[classId][0]
        );

        for (int i = 1; i <= num_models; i++) {
            printf("%3d ", result[classId][i]);
        }
        printf("\n");
    }
}

// sorts distortions in decreasing order
static void _sort_dists(sample_t *dists, int *ordp, int num_models) {
    int marked[num_models];
    for (int i = 0; i < num_models; i++) {
        marked[i] = 0;
    }
    for (int i = 0; i < num_models; i++) {
        int cmp_index = -1;
        sample_t cmp_dist = 0; // dummy initial value
        for (int j = 0; j < num_models; j++) {
            if (!marked[j] && (j == 0 || dists[j] > cmp_dist)) {
                cmp_index = j;
                cmp_dist = dists[j];
            }
        }
        if (cmp_index >= 0) {
            ordp[i] = cmp_index;
            marked[cmp_index] = 1;
        }
    }
}

int vq_classify(
        char **cb_filenames,
        int num_codebooks,
        char **prd_filenames,
        int num_predictors,
        int show_ranked_
        ) {

    assert(0 < num_codebooks && num_codebooks < MAX_MODELS);
    assert(0 < num_predictors);

    num_models = num_codebooks;
    show_ranked = show_ranked_;

    // to verify conformance
    int num_vecs;

    // quantization distortions
    sample_t dists[MAX_MODELS];

    // distortions in increasing order
    int ordp[MAX_MODELS];

    // instance to be classified
    Predictor *predictor = 0;

    printf("\nLoading models:\n");

    // load first model:

    printf("%2d: %s\n", 0, cb_filenames[0]);
    models[0] = cbook_load(cb_filenames[0]);
    if (!models[0]) {
        fprintf(stderr, "%s: error loading model\n", cb_filenames[0]);
        return 1;
    }

    num_vecs = models[0]->num_vecs;

    // load the other models:

    for (int i = 1; i < num_codebooks; ++i) {
        printf("%2d: %s\n", i, cb_filenames[i]);
        models[i] = cbook_load(cb_filenames[i]);
        if (!models[i]) {
            fprintf(stderr, "could not load model %s\n", cb_filenames[i]);
            return 1;
        }

        if (num_vecs != models[i]->num_vecs) {
            fprintf(stderr, ": conformity error.\n");
            return 1;
        }
    }

    // do classifications:

    init_results();
    printf("\n");
    for (int i = 0; i < num_predictors; ++i) {
        char *filename = prd_filenames[i];
        predictor = prd_load(filename);
        if (!predictor) {
            fprintf(stderr, "%s: error loading predictor.\n", filename);
            continue;
        }

        const int classId = get_classId(predictor->className);

        if (classId < 0) {
            fprintf(stderr, "\n%s: no model loaded for className='%s'\n",
                    filename, predictor->className);
            prd_destroy(predictor);
            continue;
        }

        result[TOTAL][0]++;
        result[classId][0]++;

        for (int r = 0; r < num_models; r++) {
            dists[r] = cbook_quantize(models[r], predictor, 0);
        }
        _sort_dists(dists, ordp, num_models);

        const int correct = classId == ordp[num_models - 1];

        // TODO make marker reflect ranking
        if (correct) {
            fprintf(stderr, GREEN("*"));
        }
        else {
            fprintf(stderr, RED("_"));
        }
        fflush(stderr);

        if (show_ranked && !correct) {
            printf("\n%s: '%s'\n", filename, predictor->className);

            int index = 0;
            for (int r = num_models - 1; r >= 0; r--, index++) {
                const char *mark = classId == ordp[r] ? "*" : "";
                printf("  [%2d] %1s %-60s : %e  : '%s'\n",
                        index,
                        mark,
                        cb_filenames[ordp[r]],
                        dists[ordp[r]],
                        models[ordp[r]]->className
                );

                // only show until corresponding model:
                if (classId == ordp[r]) {
                    break;
                }
            }
            printf("\n");
        }

        // capture test:classification occurrence:

        confusion[classId][ordp[num_models - 1]]++;

        // did best candidate correctly classify the instance?
        if (ordp[num_models - 1] == classId) {
            result[TOTAL][1]++;
            result[classId][1]++;
        }
        else {
            // update order of recognized candidate:
            for (int i = 1; i < num_models; i++) {
                if (ordp[num_models - 1 - i] == classId) {
                    result[TOTAL][i + 1]++;
                    result[classId][i + 1]++;
                    break;
                }
            }
        }

        prd_destroy(predictor);
    }

    report_results();
    return 0;
}
