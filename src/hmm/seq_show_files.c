/* seq_show_files.c -- ECOZ System
 */

#include "hmm.h"
#include "symbol.h"
#include "utl.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


int seq_show_files(
        int with_prob,
        int gen_Qopt,
        int show_sequence,
        char* hmm_filename,
        char* seq_filenames[],
        int num_seq_filenames
        ) {

    Hmm *hmm = 0;

    if (with_prob || gen_Qopt) {
        assert(hmm_filename != 0);

        hmm = hmm_load(hmm_filename);
        if (!hmm) {
            printf("%s: error loading model\n", hmm_filename);
            return 1;
        }
        printf("Loaded model for class: '%s'\n", hmm->className);
    }

    for (int i = 0; i < num_seq_filenames; ++i) {
        char *nom_cad = seq_filenames[i];
        int T, M;
        char className[MAX_CLASS_NAME_LEN];
        Symbol *seq = seq_load(nom_cad, &T, &M, className);
        if (!seq) {
            printf("error with %s\n", nom_cad);
            free(seq);
            continue;
        }

        printf("\n%8s : '%s'", nom_cad, className);

        if (show_sequence) {
            printf("\n  M=%-4d O = ", M);
            seq_show(seq, T);
            printf("\n");
        }

        if (with_prob) {
            prob_t prob = hmm_log_prob(hmm, seq, T);
            printf(" log(P) = %Le", prob);
        }

        if (gen_Qopt) {
            int *Qopt = (int *) new_vector(T, sizeof(int));
            prob_t probQopt = hmm_genQopt(hmm, seq, T, Qopt);

            printf("\n  N=%-4d Q* = «", hmm->N);
            char *comma = "";
            for (int t = 0; t < T; t++) {
                printf("%s%3d", comma, Qopt[t]);
                comma = ", ";
            }
            printf("»\n");

            if (with_prob) {
                printf(" log(P*) = %Le", probQopt);
            }

            free(Qopt);
        }
        free(seq);

        printf("\n");
    }
    return 0;
}
