/* hmm.show.c -- ECOZ System
 */

#include "hmm.h"

#include <stdio.h>
#include <getopt.h>

static char *default_format = "%g ";


static void usage() {
    printf("\
\n\
ECOZ System\n\
hmm.show - show a model\n\
\n\
    hmm.show [-f formato] <hmm>\n\
\n\
Default format: \"%s\"\n\
\n\
Example:\n\
\n\
    hmm.show -f \"%%f \" model.hmm\n\
\n",
           default_format
    );
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage();
        return 1;
    }
    char *format = default_format;

    int opc;
    while (EOF != (opc = getopt(argc, argv, "f:"))) {
        switch (opc) {
            case 'f':
                format = optarg;
                break;
            case '?':
                return 0;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "model name missing.\n");
        return 1;
    }

    char *filename = argv[optind];

    return hmm_show(filename, format);
}
