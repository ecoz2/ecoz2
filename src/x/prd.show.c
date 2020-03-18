/* prd.show.c -- ECOZ System
 */

#include "utl.h"
#include "lpc.h"

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>


static void usage() {
    printf("\
\n\
ECOZ System\n\
\n\
	prd.show - Show predictor\n\
\n\
	prd.show [ options ] <predictor>\n\
\n\
    Options:\n\
      -k             Show reflection coefficients\n\
      -r from-to     Coefficient range selection\n\
\n"
    );
}

int main(int argc, char *argv[]) {
    int show_reflections = 0;
    int from = -1;
    int to = -1;

    int opc;
    while (EOF != (opc = getopt(argc, argv, "kr:h"))) {
        switch (opc) {
            case 'k':
                show_reflections = 1;
                break;
            case 'r':
                if (sscanf(optarg, "%d-%d", &from, &to) == 0 || from < 0 || from > to) {
                    fprintf(stderr, "invalid range: '%s'\n", optarg);
                    return 1;
                }
                break;
            case 'h': case '?':
                usage();
                return 0;
        }
    }

    if (optind >= argc) {
        printf("missing predictor file\n");
        usage();
        return 1;
    }

    char *filename = argv[optind];

    return prd_show_file(
            filename,
            show_reflections,
            from,
            to
    );
}
