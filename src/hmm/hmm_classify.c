/* hmm.classify.c -- ECOZ System
 */

#include "hmm.h"
#include "utl.h"
#include "list.h"
#include "vq.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <float.h>
#include <assert.h>

//#undef PAR
#define PAR 1

#ifdef PAR
#include <omp.h>
#endif

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

typedef struct {
    float accuracy;
    float avg_accuracy;
} Summary;


static void _report_summary(FILE *file, Summary *summary) {
    fprintf(file, "{\n"
                  "  \"accuracy\":     %.2f,\n"
                  "  \"avg_accuracy\": %.2f\n"
                  "}\n",
            summary->accuracy,
            summary->avg_accuracy
    );
}

static void report_summary(Summary *summary) {
    //_report_summary(stdout, summary);

    char filename[2048];
    sprintf(filename, "classification.json");
    FILE *file = fopen(filename, "w");
    if (file) {
        _report_summary(file, summary);
        fclose(file);
    }
    else {
        fprintf(stderr, "ERROR creating %s\n", filename);
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

    int num_classes = 0;

    Summary summary = { 0, 0 };

    for (int classId = 0; classId <= TOTAL; classId++) {
        if (result[classId][0] == 0)
            continue;

        const int num_tests = result[classId][0];
        const int correct_tests = result[classId][1];

        const float acc = (float) correct_tests / num_tests;

        if (classId < TOTAL) {
            ++num_classes;
            summary.avg_accuracy += acc;

            printf("%*s ", margin, models[classId]->className);
            printf("  %3d    ", classId);
        }
        else {
            printf("\n");
            printf("%*s ", margin, "");
            printf("  TOTAL  ");
            summary.accuracy = acc;
        }

        printf("  %6.2f%%   %3d        ",
               (float) (100.0 * acc),
               num_tests
        );

        for (int i = 1; i <= num_models; i++) {
            printf("%3d ", result[classId][i]);
        }
        printf("\n");
    }

    summary.accuracy *= 100.;
    summary.avg_accuracy = summary.avg_accuracy * 100. / num_classes;

    printf("  avg_accuracy  %6.2f%%\n", summary.avg_accuracy);
    //printf("    error_rate  %6.2f%%\n", (float) (100. - avg_accuracy));
    printf("\n");

    report_summary(&summary);
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

static FILE *c12n_file = 0;

void c12n_prepare(const char *classification_filename, int codebook_size, int num_seqs) {
    if (!classification_filename) {
        return;
    }

    if (0 == (c12n_file = fopen(classification_filename, "w"))) {
        printf(RED("error creating %s\n"), classification_filename);
        return;
    }

    fprintf(c12n_file, "# num_models=%d  M=%d  num_seqs=%d\n",
            num_models, codebook_size, num_seqs
            );

    fprintf(c12n_file, "%s,%s,%s,%s",
            "seq_filename", "seq_class_name",
            "correct", "rank"
    );

    for (int r = 1; r <= num_models; ++r) {
        fprintf(c12n_file, ",r%d", r);
    }
    fprintf(c12n_file, "\n");
}

void c12n_add_case(const char *seq_filename, const char *seq_class_name,
        int correct,
        int rank,
        int ranked_model_ids[]
        ) {
    if (!c12n_file) {
        return;
    }

    fprintf(c12n_file, "%s,%s,%s,%d",
            seq_filename, seq_class_name,
            correct ? "*" : "!",
            rank
            );

    for (int r = 1; r <= num_models; ++r) {
        int model_id = ranked_model_ids[r];
        char *model_class = models[model_id]->className;
        fprintf(c12n_file, ",%s", model_class);
    }

    fprintf(c12n_file, "\n");

}

void c12n_close(void) {
    if (c12n_file) {
        fclose(c12n_file);
        c12n_file = 0;
    }
}



int hmm_classify(
        char **model_names,
        unsigned num_model_names,

        // if giving sequences directly:
        char **seq_filenames,
        unsigned num_seq_filenames,

        // if giving predictors:
        char **cb_filenames,
        int num_codebooks,
        char **prd_filenames,
        int num_predictors,

        int show_ranked_,
        const char *classification_filename
        ) {

    assert(0 < num_model_names && num_model_names < MAX_MODELS);

    num_models = num_model_names;
    show_ranked = show_ranked_;

    // codebook size
    // to verify conformance
    int Mcmp;

    // probabilities on a sequence
    prob_t probs[MAX_MODELS];

    // probabilities in increasing order
    int ordp[MAX_MODELS] = {0};

    static HmmProb *hmmprob_objects[MAX_MODELS];

    printf("\nLoading HMM models:\n");

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

    for (unsigned i = 1; i < num_model_names; ++i) {
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

    SeqProvider *sp = seq_provider_create(
            num_models,
            Mcmp,
            seq_filenames,
            num_seq_filenames,
            cb_filenames,
            num_codebooks,
            prd_filenames,
            num_predictors
    );

    const int with_direct_sequences = seq_provider_with_direct_sequences(sp);

    if (!with_direct_sequences) {
        // check HMM-codebook correspondence
        for (int r = 0; r < num_models; ++r) {
            Hmm *hmm = models[r];
            Codebook *cb = sp->codebooks[r];
            assert(strcmp(hmm->className, cb->className) == 0);
        }
    }

    c12n_prepare(classification_filename, Mcmp, sp->num_sequences);

    printf("\n");

    const double measure_start_sec = measure_time_now_sec();

    while (seq_provider_has_next(sp)) {
        NextSeq *next_seq = seq_provider_get_next(sp);
        if (!next_seq) {
            continue;
        }

        int classId = -1;

        if (with_direct_sequences) {
            classId = get_classId(next_seq->seq_class_name);
            if (classId < 0) {
                continue;
            }

#ifdef PAR
#pragma omp parallel for
#endif
            for (int r = 0; r < num_models; r++) {
                // probabilities for the given sequence:
                probs[r] = hmmprob_log_prob(hmmprob_objects[r],
                                            next_seq->sequence,
                                            next_seq->T);
            }
        }
        else {
            classId = get_classId(next_seq->prd_class_name);
            if (classId < 0) {
                continue;
            }

#ifdef PAR
#pragma omp parallel for
#endif
            for (int r = 0; r < num_models; r++) {
                // probabilities for the corresponding sequences
                // resulting from quantizing the given predictor:
                probs[r] = hmmprob_log_prob(hmmprob_objects[r],
                                            next_seq->sequences[r],
                                            next_seq->T);
            }
        }

        result[TOTAL][0]++;
        result[classId][0]++;

        _sort_probs(probs, ordp, num_models);

        const int correct = classId == ordp[num_models - 1];

        // TODO make marker reflect ranking
        if (correct) {
            fprintf(stderr, GREEN("*"));
        }
        else {
            fprintf(stderr, RED("!"));
        }
        fflush(stderr);

        const int do_show_ranked = show_ranked && !correct;

        if (do_show_ranked) {
            if (with_direct_sequences) {
                printf("\n%s: '%s'\n", next_seq->seq_filename, next_seq->seq_class_name);
            }
            else {
                printf("\n%s: '%s'\n", next_seq->prd_filename, next_seq->prd_class_name);
            }
        }

        int best_rank = 1;
        int ranked_model_ids[num_models + 1];  // +1 for consistency with first rank @ [1]
        int correct_model_shown = 0;
        int rank = 1;
        for (int r = num_models - 1; r >= 0; r--, rank++) {
            const int model_id = ordp[r];

            if (do_show_ranked && !correct_model_shown) {
                const char *mark = classId == model_id ? "*" : "";
                printf("  [%2d] %1s <%2d>",
                       rank,
                       mark,
                       model_id
                );
                fflush(stdout);

                printf("  %-60s : %Le  : '%s'\n",
                       model_names[model_id],
                       (long double) probs[model_id],
                       models[model_id]->className
                );
            }

            ranked_model_ids[rank] = model_id;

            // only the display above until corresponding model:
            if (classId == model_id) {
                best_rank = rank;
                correct_model_shown = 1;
            }
        }
        if (do_show_ranked) {
            printf("\n");
        }

        // capture test:classification occurrence:

        confusion[classId][ordp[num_models - 1]]++;

        // did best candidate correctly classify the instance?
        if (correct) {
            result[TOTAL][1]++;
            result[classId][1]++;
        }
        else {
            // update order of recognized candidate:
            for (int r = 1; r < num_models; r++) {
                if (ordp[num_models - 1 - r] == classId) {
                    result[TOTAL][r + 1]++;
                    result[classId][r + 1]++;
                    break;
                }
            }
        }

        if (with_direct_sequences) {
            c12n_add_case(next_seq->seq_filename, next_seq->seq_class_name,
                    correct, best_rank, ranked_model_ids);
        }
        else {
            c12n_add_case(next_seq->prd_filename, next_seq->prd_class_name,
                    correct, best_rank, ranked_model_ids);
        }
    }

    seq_provider_destroy(sp);

    const double measure_elapsed_sec = measure_time_now_sec() - measure_start_sec;

    c12n_close();

    release_not_loaded_models_list();

    report_results();

    for (int r = 0; r < num_models; r++) {
        hmmprob_destroy(hmmprob_objects[r]);
        hmm_destroy(models[r]);
    }

    printf("=> classification took %.2fs\n\n", measure_elapsed_sec);
    return 0;
}
