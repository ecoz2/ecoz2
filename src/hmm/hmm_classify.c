/* hmm.classify.c -- ECOZ System
 */

#include "hmm.h"
#include "utl.h"
#include "list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <float.h>
#include <assert.h>

#define GREEN(s) "\x1b[32m" s "\x1b[0m"
#define RED(s)   "\x1b[31m" s "\x1b[0m"

#define MAX_MODELS    256

#define MAX_CLASSES   128

// loaded models
static Hmm *models[MAX_MODELS];

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

static List not_loaded_models; // to report each only once

static void create_not_loaded_models_list() {
    not_loaded_models = list_create();
}

static void release_not_loaded_models_list() {
    for (int i = 0; i < list_size(not_loaded_models); i++) {
        free(list_elementAt(not_loaded_models, i));
    }
}

static int not_loaded_model_reported(const char *className) {
    for (int i = 0; i < list_size(not_loaded_models); i++) {
        const char *cn = (const char *) list_elementAt(not_loaded_models, i);
        if (strcmp(className, cn) == 0) {
            return 1;
        }
    }
    return 0;
}

static int get_classId(const char *className) {
    for (int i = 0; i < num_models; i++) {
        if (0 == strcmp(className, models[i]->className)) {
            return i;
        }
    }
    if (not_loaded_model_reported(className)) {
        return -1;
    }
    else {
        list_addElement(not_loaded_models, (void *) strdup(className));
        fprintf(stderr, "\nNo model loaded for '%s'\n", className);
        return -2;
    }
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

// sorts probabilities in increasing order
static void _sort_probs(prob_t *probs, int *ordp, int num_models) {
    int marked[num_models];
    for (int i = 0; i < num_models; i++) {
        marked[i] = 0;
    }
    for (int i = 0; i < num_models; i++) {
        int cmp_index = -1;
        prob_t prob = 0; // dummy initial value
        for (int j = 0; j < num_models; j++) {
            if (!marked[j] && (j == 0 || probs[j] < prob)) {
                cmp_index = j;
                prob = probs[j];
            }
        }
        if (cmp_index >= 0) {
            ordp[i] = cmp_index;
            marked[cmp_index] = 1;
        }
    }
}


int hmm_classify(
        char **model_names,
        int num_model_names,
        char **seq_filenames,
        int num_seq_filenames,
        int show_ranked_
        ) {

    assert(0 < num_model_names && num_model_names < MAX_MODELS);
    assert(0 < num_seq_filenames);

    num_models = num_model_names;
    show_ranked = show_ranked_;

    // sequence length
    int T;

    // codebook size
    int M;

    // to verify conformance
    int Mcmp;

    // probabilities on a sequence
    prob_t probs[MAX_MODELS];

    // probabilities in increasing order
    int ordp[MAX_MODELS];

    // sequence to be classified
    Symbol *sequence = 0;

    // className of the sequence
    char seq_className[MAX_CLASS_NAME_LEN];

    static HmmProb *hmmprob_objects[MAX_MODELS];

    printf("\nLoading models:\n");

    // load first model:

    printf("%2d: %s\n", 0, model_names[0]);
    models[0] = hmm_load(model_names[0]);
    if (!models[0]) {
        fprintf(stderr, "%s: error loading model\n", model_names[0]);
        return 1;
    }

    Mcmp = models[0]->M;

    hmmprob_objects[0] = hmmprob_create(models[0]);

    // load the other models:

    for (int i = 1; i < num_model_names; ++i) {
        printf("%2d: %s\n", i, model_names[i]);
        models[i] = hmm_load(model_names[i]);
        if (!models[i]) {
            fprintf(stderr, "could not load model %s\n", model_names[i]);
            return 1;
        }

        if (Mcmp != models[i]->M) {
            fprintf(stderr, ": conformity error.\n");
            return 1;
        }

        hmmprob_objects[i] = hmmprob_create(models[i]);
    }

    // do classifications:

    create_not_loaded_models_list();

    init_results();
    printf("\n");
    for (int i = 0; i < num_seq_filenames; ++i) {
        const char *seq_filename = seq_filenames[i];
        sequence = seq_load(seq_filename, &T, &M, seq_className);
        if (!sequence) {
            fprintf(stderr, "%s: error loading sequence.\n", seq_filename);
            continue;
        }
        if (Mcmp != M) {
            fprintf(stderr, "%s: conformity error.\n", seq_filename);
            free(sequence);
            continue;
        }

        const int classId = get_classId(seq_className);

        if (classId < 0) {
            free(sequence);
            continue;
        }

        result[TOTAL][0]++;
        result[classId][0]++;

        for (int r = 0; r < num_models; r++) {
            probs[r] = hmmprob_log_prob(hmmprob_objects[r], sequence, T);
        }
        _sort_probs(probs, ordp, num_models);

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
            printf("\n%s: '%s'\n", seq_filename, seq_className);

            int index = 0;
            for (int r = num_models - 1; r >= 0; r--, index++) {
                const char *mark = classId == ordp[r] ? "*" : "";
                printf("  [%2d] %1s %-60s : %Le  : '%s'\n",
                        index,
                        mark,
                        model_names[ordp[r]],
                        probs[ordp[r]],
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

        free(sequence);
    }
    release_not_loaded_models_list();

    report_results();

    for (int r = 0; r < num_models; r++) {
        hmmprob_destroy(hmmprob_objects[r]);
        hmm_destroy(models[r]);
    }

    return 0;
}
