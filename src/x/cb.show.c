/* cb.show.c -- ECOZ System
 */

#include "utl.h"
#include "vq.h"

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

static void usage() {
    printf("\
\n\
ECOZ System\n\
\n\
	cb.show - Show codebook\n\
\n\
	cb.show <codebook> ...\n\
\n"
    );
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage();
        return 1;
    }

    int P;
    int size;
    char className[MAX_CLASS_NAME_LEN];

    for (int i = 1; i < argc; i++) {
        char *filename = argv[i];
        printf("%s:\n", filename);

        sample_t *reflections = cb_load(filename, &P, &size, className);
        if (!reflections) {
            printf(": error loading %s.\n", argv[i]);
            continue;
        }

        printf("P=%d  size=%d  className='%s'\n", P, size, className);
        sample_t *refl = reflections;
        for (int v = 0; v < size; v++, refl += (1 + P)) {
            printf("  [%d]: [", v);
            char *comma = "";
            for (int p = 0; p < P + 1; p++) {
                printf("%s%g", comma, refl[p]);
                comma = ", ";
            }
            printf("]\n");
        }
        free(reflections);

        printf("\n");
    }

    return 0;
}
