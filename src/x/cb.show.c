/* cb.show.c -- ECOZ System
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
	cb.show - Show codebook\n\
\n\
	cb.show [ options ] <codebook>\n\
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

    char *filename = argv[optind];
    int P;
    int size;
    char className[MAX_CLASS_NAME_LEN];

    sample_t *reflections = cb_load(filename, &P, &size, className);
    if (!reflections) {
        printf(": error loading %s.\n", filename);
        return 2;
    }

    if (from < 0) {
        from = 1;
    }
    if (to < 0 || to > P) {
        to = P;
    }

    printf("# %s:\n", filename);
    printf("# P=%d  size=%d  className='%s'\n", P, size, className);

    const char *comma = "";
    for (int k = from; k <= to; k++) {
        printf("%sk%d", comma, k);
        comma = ",";
    }
    printf("\n");

    sample_t *refl = reflections;
    for (int v = 0; v < size; v++, refl += (1 + P)) {
        assert(refl[0] == 0);
        comma = "";
        for (int k = from; k <= to; k++) {
            printf("%s%g", comma, refl[k]);
            comma = ",";
        }
        printf("\n");
    }
    free(reflections);
    printf("\n");

    return 0;
}
