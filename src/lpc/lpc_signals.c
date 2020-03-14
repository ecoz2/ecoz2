/* lpc_signals.c -- ECOZ System
 */

#include "lpc.h"

#include "sgn.h"
#include "utl.h"
#include "list.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

typedef struct {
    char className[MAX_CLASS_NAME_LEN];
    List sgn_filenames;
} ByClass;

static ByClass *findOrCreate(List byClasses, const char *className) {
    ByClass *e;
    long numClasses = list_size(byClasses);
    for (long i = 0; i < numClasses; i++) {
        e = (ByClass *) list_elementAt(byClasses, i);
        if (0 == strcmp(e->className, className)) {
            return e;
        }
    }
    e = (ByClass *) malloc(sizeof(ByClass));
    assert(e);
    strcpy(e->className, className);
    e->sgn_filenames = list_create();
    list_addElement(byClasses, e);
    return e;
}

int lpc_signals(
        int P,
        int windowLengthMs,
        int offsetLengthMs,
        int minpc,
        float split,
        char *sgn_filenames[],
        int num_signals
        ) {

    // predictor to be generated:
    char prd_filename[2048];

    List byClasses = list_create();

    for (int i = 0; i < num_signals; ++i) {
        char *sgn_filename = sgn_filenames[i];
        char className[MAX_CLASS_NAME_LEN] = "";
        get_class_name(sgn_filename, className, sizeof(className));
        ByClass *e = findOrCreate(byClasses, className);
        list_addElement(e->sgn_filenames, sgn_filename);
    }

    // process the collected classes:

    printf("Number of classes: %ld\n", list_size(byClasses));

    const long numClasses = list_size(byClasses);
    for (long i = 0; i < numClasses; i++) {
        ByClass *e = (ByClass *) list_elementAt(byClasses, i);

        list_shuffle(e->sgn_filenames);
        long numSignals = list_size(e->sgn_filenames);

        if (minpc > 0 && numSignals < minpc) {
            printf("class '%s': insufficient #signals=%ld\n", e->className, numSignals);
            continue;
        }

        int numTrain = -1;
        if (split > 0) {
            numTrain = (int) (split * numSignals);
        }

        printf("class '%s': %ld\n", e->className, numSignals);

        for (long j = 0; j < numSignals; j++) {
            char *sgn_filename = (char *) list_elementAt(e->sgn_filenames, j);
            printf("  %s\n", sgn_filename);

            Sgn *sgn = sgn_load(sgn_filename);
            if (!sgn) {
                printf("%s: error loading signal\n", sgn_filename);
                continue;
            }

            //printf("\n<%s>:\n", sgn_filename);
            //sgn_show(sgn);

            Predictor *predictor = lpaOnSignal(P, windowLengthMs, offsetLengthMs, sgn);
            if (!predictor) {
                sgn_destroy(sgn);
                printf("cannot create lpc predictor\n");
                continue;
            }

            const char *base_dir = "predictors";
            if (numTrain > 0) {
                if (j <= numTrain) {
                    base_dir = "predictors/TRAIN";
                }
                else {
                    base_dir = "predictors/TEST";
                }
            }
            get_output_filename(sgn_filename, base_dir, ".prd", prd_filename);

            //printf("<%s>:\n", prd_filename);

            strcpy(predictor->className, e->className);

            if (prd_save(predictor, prd_filename)) {
                printf("%s: error saving predictor\n", prd_filename);
            }
            else {
                printf("%s: '%s': predictor saved\n", prd_filename, predictor->className);
            }

            prd_destroy(predictor);
            sgn_destroy(sgn);
            printf("\n");
        }
    }

    for (long i = 0; i < numClasses; i++) {
        ByClass *e = (ByClass *) list_elementAt(byClasses, i);
        list_destroy(e->sgn_filenames);
    }
    list_destroy(byClasses);

    return 0;
}
