/* report.c
 */

#include "lpc.h"

#include <stdio.h>
#include <string.h>

static FILE *file = 0;

void prepare_report(char *nom_rpt, long tot_vecs, double eps) {
    printf("Report: %s\n", nom_rpt);

    if (0 == (file = fopen(nom_rpt, "w"))) {
        printf("error creating report\n");
    }
    else {
        fprintf(file, "\
Codebook generation\n\n\
%ld training vectors. (ε = %g)\n\n",
                tot_vecs, eps
        );
    }
}

void report_cbook(char *filename, int num_pas,
                  sample_t DDprm, sample_t sigma, int M, int *cardd, sample_t *discel) {

    if (!file) {
        return;
    }

    fprintf(file, "### %s (%d passes)  Dprm = %f  σ = %f\n",
            filename, num_pas, DDprm, sigma);

    int emptyCells = 0;
    for (int i = 0; i < M; i++) {
        if (!cardd[i]) {
            emptyCells++;
        }
    }

    fprintf(file, "Cardinalities:");
    if (emptyCells) {
        fprintf(file, "  (%d empty cells)", emptyCells);
    }
    fprintf(file, "\n");
    for (int i = 0; i < M; i++) {
        fprintf(file, "%8d ", cardd[i]);
    }
    fprintf(file, "\n");

    fprintf(file, "average cell distortion:\n");
    for (int i = 0; i < M; i++) {
        if (cardd[i]) {
            fprintf(file, "%8.3g ", discel[i] / cardd[i]);
        }
        else {
            fprintf(file, "%8s ", "_");
        }
    }
    fprintf(file, "\n\n");
    fflush(file);
}

void close_report() {
    if (0 != file) {
        fclose(file);
    }
    file = 0;
}
