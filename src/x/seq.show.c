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

    char *hmm_filename = 0;

    if (with_prob || gen_Qopt) {
        hmm_filename = argv[optind++];
    }

    const int num_seq_filenames = argc - optind;
    char **seq_filenames = argv + optind;

    return seq_show_files(
            with_prob,
            gen_Qopt,
            show_sequence,
            hmm_filename,
            seq_filenames,
            num_seq_filenames
    );
}
