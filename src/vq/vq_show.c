/* vq_show.c -- ECOZ System
 */

#include "utl.h"
#include "vq.h"

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


int vq_show(char *codebook_filename, int from, int to) {
    int P;
    int size;
    char className[MAX_CLASS_NAME_LEN];

    sample_t *reflections = cb_load(codebook_filename, &P, &size, className);
    if (!reflections) {
        printf(": error loading %s.\n", codebook_filename);
        return 2;
    }

    if (from < 0) {
        from = 1;
    }
    if (to < 0 || to > P) {
        to = P;
    }

    printf("# %s:\n", codebook_filename);
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
