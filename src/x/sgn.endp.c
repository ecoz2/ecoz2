/* sgn.endp.c -- ECOZ System
 */

#include "sgn.h"
#include "utl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage() {
    printf("\
\n\
ECOZ System\n\
sgn.endp - Endpoint detection\n\
\n\
	sgn.endp <wav-file> ...\n\
\n\
   For each given file <name>.wav, the resulting file takes\n\
   the name <name>__S<start>_L<length>$.wav, where <start>\n\
   is the start index of the detection wrt to input signal,\n\
   and <length> is the size of the detection.\n\
\n"
    );
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage();
        return 0;
    }

    int num_processed = 0;
    double max_percent = 0;
    double min_percent = 0;
    double total_percents = 0;

    for (int i = 1; i < argc; i++) {
        char *filename = argv[i];
        Sgn *s = sgn_load(filename);
        if (!s) {
            printf("%s: error loading signal\n", filename);
            continue;
        }

        printf("%s:\n", filename);
        //sgn_show(s);

        long start;
        Sgn *e = sgn_endpoint(s, &start);
        if (e) {
            char no_ext[2048];
            strcpy(no_ext, filename);
            camext(no_ext, "");
            char e_filename[2048];
            #pragma GCC diagnostic ignored "-Wformat-overflow"
            sprintf(e_filename, "%s__S%ld_L%d$.wav", no_ext, start, e->numSamples);
            sgn_save(e, e_filename);

            double percent = 100. * e->numSamples / s->numSamples;
            total_percents += percent;
            if (!num_processed || max_percent < percent) {
                max_percent = percent;
            }
            if (!num_processed || min_percent > percent) {
                min_percent = percent;
            }
            num_processed++;
        }
        else {
            printf("  No resulting endpoint detection.\n");
        }
        printf("\n");

        sgn_destroy(s);
    }

    if (num_processed) {
        printf("%d processed.  %.1f%% average new size.  min=%.1f%% max=%.1f%%\n",
                num_processed, total_percents / num_processed,
                min_percent, max_percent);
    }

    return 0;
}
