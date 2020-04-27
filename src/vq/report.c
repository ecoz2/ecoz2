/* report.c
 */

#include "lpc.h"
#include "vq.h"

#include <stdio.h>
#include <string.h>

static long tot_vecs = 0;
static FILE *file = 0;
static FILE *file_csv = 0;

void prepare_report(char *rpt_filename, long tot_vecs_, double eps) {
    tot_vecs = tot_vecs_;

    printf("Report: %s\n", rpt_filename);

    if (0 == (file = fopen(rpt_filename, "w"))) {
        printf("error creating report file %s\n", rpt_filename);
    }

    if (file) {
        fprintf(file, "Codebook generation\n\n%ld training vectors. (ε = %g)\n\n", tot_vecs, eps);
    }

    char csv_filename[2048];
    #pragma GCC diagnostic ignored "-Wformat-overflow"
    sprintf(csv_filename, "%s.csv", rpt_filename);
    if (0 == (file_csv = fopen(csv_filename, "w"))) {
        printf("error creating report file %s\n", csv_filename);
    }
    else {
        fprintf(file_csv, "# %ld training vectors. (ε = %g)\n", tot_vecs, eps);
        fprintf(file_csv, "%s,%s,%s,%s,%s\n", "M", "passes", "DDprm", "σ", "inertia");
    }
}

void report_cbook(char *cb_filename, int num_pas,
                  sample_t DDprm, sample_t sigma, sample_t inertia,
                  int M, int *cardd, sample_t *discel,
                  CodewordAndMinDist *minDists
                  ) {

    if (!file) {
        return;
    }

    fprintf(file, "### %s (%2d passes)  Dprm = %f  σ = %f  inertia = %f\n",
            cb_filename, num_pas, DDprm, sigma, inertia);

    if (file_csv) {
        fprintf(file_csv, "%d,%d,%f,%f,%f\n", M, num_pas, DDprm, sigma, inertia);
        fflush(file_csv);
    }

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

    // CELLS
    char cells_csv[2048];
    FILE *cells_file;
    #pragma GCC diagnostic ignored "-Wformat-overflow"
    sprintf(cells_csv, "%s.cards_dists.csv", cb_filename);
    if (0 == (cells_file = fopen(cells_csv, "w"))) {
        printf("error creating %s\n", cells_csv);
    }
    else {
        fprintf(cells_file, "%s,%s,%s\n", "index", "cardinality", "distortion");
        for (int i = 0; i < M; i++) {
            fprintf(cells_file, "%d,%d,", i, cardd[i]);
            if (cardd[i] > 0) {
                sample_t dist = cardd[i] > 0 ? discel[i] / cardd[i] : 0;
                fprintf(cells_file, "%f\n", dist);
            }
            else {
                fprintf(cells_file, "NaN\n");
            }
        }
        fclose(cells_file);
    }

    // MIN DISTORTIONS
    char min_dists_csv[2048];
    FILE *min_dists_file;
    #pragma GCC diagnostic ignored "-Wformat-overflow"
    sprintf(min_dists_csv, "%s.min_dists.csv", cb_filename);
    if (0 == (min_dists_file = fopen(min_dists_csv, "w"))) {
        printf("error creating %s\n", min_dists_csv);
    }
    else {
        fprintf(min_dists_file, "%s\n", "codeword,minDistortion");
        for (int v = 0; v < tot_vecs; v++) {
            fprintf(min_dists_file, "%d,%f\n", minDists[v].codeword, minDists[v].minDist);
        }
        fclose(min_dists_file);
    }
}

void close_report() {
    if (file) {
        fclose(file);
        file = 0;
    }
    if (file_csv) {
        fclose(file_csv);
        file_csv = 0;
    }
}
