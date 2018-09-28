/* sgn.show.c -- ECOZ System
 */

#include "sgn.h"

#include <stdio.h>
#include <stdlib.h>

static void usage() {
    printf("\
\n\
ECOZ System\n\
sgn.show - show audio signal\n\
\n\
	sgn.show <wav-file>\n\
	sgn.show <wav-file> <#samples>\n\
	sgn.show <wav-file> <from> <to>\n\
\n"
    );
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage();
        return 0;
    }
    char *filename = argv[1];
    Sgn *s = sgn_load(filename);
    if (!s) {
        printf("error loading signal\n");
        return 1;
    }

    printf("%s:\n", filename);
    sgn_show(s);

    int show_samples = 0;
    int from = 0;
    int to = s->numSamples -1;
    if (argc >= 4) {
        from = atoi(argv[2]);
        to = atoi(argv[3]);
        show_samples = 1;
    }
    else if (argc == 3) {
        from = 0;
        to = atoi(argv[2]);
        show_samples = 1;
    }
    if (to >= s->numSamples) {
        to = s->numSamples -1;
    }

    if (from >= 0 && from <= to) {
        double sum = 0;
        double max = 0;
        double min = 0;
        for (int i = from; i <= to; i++) {
            sample_t val = s->samples[i];
            sum += val;
            if (i == from || max < val) {
                max = val;
            }
            if (i == from || min > val) {
                min = val;
            }
            if (show_samples) {
                printf(" [%2d] %g\n", i, val);
            }
        }
        const double mean = sum / s->numSamples;
        printf("[%d..%d]: mean=%g max=%g min=%g\n",
                from, to, mean, max, min);
    }

    sgn_destroy(s);
    return 0;
}
