/* vq.show.c -- ECOZ System
 */

#include "utl.h"
#include "vq.h"

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <getopt.h>


static void usage() {
    printf("\
\n\
ECOZ System\n\
\n\
	vq.show - Show codebook\n\
\n\
	vq.show [ options ] <codebook>\n\
\n\
    Options:\n\
      -r from-to     Coefficient range selection\n\
\n"
    );
}

int main(int argc, char *argv[]) {
    int from = -1;
    int to = -1;

    int opc;
    while (EOF != (opc = getopt(argc, argv, "r:h"))) {
        switch (opc) {
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
        printf("missing codebook file\n");
        usage();
        return 1;
    }

    char *codebook_filename = argv[optind];

    return vq_show(codebook_filename, from, to);
}
