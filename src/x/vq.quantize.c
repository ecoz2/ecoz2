/* vq.quantize.c -- ECOZ System
 */

#include "utl.h"
#include "lpc.h"
#include "vq.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

static void usage() {
    printf("\
\n\
ECOZ System\n\
Vector quantization\n\
\n\
	vq.quantize <codebook> <predictor> ...\n\
\n\
	<codebook>	 reference codebook for quantization.\n\
\n\
	<predictor> ...	LPC vector sequences to be quantized.\n\
\n\n"
    );
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        usage();
        return 0;
    }

    char *nom_raas = argv[1];

    const int num_predictors = argc - 2;
    char **predictor_filenames = argv + 2;

    const int show_filenames = 1;

    return vq_quantize(nom_raas, (const char **) predictor_filenames, num_predictors, show_filenames);
}
