/* seq.show.c -- ECOZ System
 */

#include "hmm.h"
#include "symbol.h"
#include "utl.h"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

static void usage() {
    printf("\
\n\
ECOZ System\n\
Observation and state sequence description\n\
\n\
	seq.show [[-P] [-Q] <hmm>] [-c] <sequence> ...\n\
\n\
	-P              show associated probabilities\n\
	-Q              show most likely state sequence\n\
	<hmm>           required if -P or -Q given\n\
	-c              do not show sequence\n\
	<sequence> ...  observation sequences\n\
\n"
    );
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage();
        return 1;
    }

    int with_prob = 0;
    int gen_Qopt = 0;
    int show_sequence = 1;
    int test = 0;

    int opc;
    while (EOF != (opc = getopt(argc, argv, "cPQt"))) {
        switch (opc) {
            case 'c':
                show_sequence = 0;
                break;
            case 'P':
                with_prob = 1;
                break;
            case 'Q':
                gen_Qopt = 1;
                break;
            case 't':
                test = 1;
                break;
            case '?':
                return 1;
        }
    }

    if (test) {
        char *filename = "__temp.seq";
        int T = 20, M = 4;
        Symbol *seq = seq_create(T);
        for (int t = 0; t < T; t++) {
            seq[t] = t % M;
        }
        seq_show(seq, T);
        seq_save(seq, T, M, "clasita", filename);

        char className[MAX_CLASS_NAME_LEN];
        seq = seq_load(filename, &T, &M, className);
        seq_show(seq, T);

        return 0;
    }

    Hmm *hmm = 0;

    if (with_prob || gen_Qopt) {
        char *hmm_filename = argv[optind++];
        hmm = hmm_load(hmm_filename);
        if (!hmm) {
            printf("%s: error loading model\n", hmm_filename);
            return 1;
        }
        printf("Loaded model for class: '%s'\n", hmm->className);
    }

    for (int i = optind; i < argc; i++) {
        char *nom_cad = argv[i];
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
