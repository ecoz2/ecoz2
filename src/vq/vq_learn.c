/* vq.learn.c -- ECOZ System
 * Codebook generation according to Juang et al (1982).
 */

#include "vq_learn.i"

static void learn(const char *codebook_class_name,
                  int P,
                  char *prefix,
                  int num_raas,
                  long tot_vecs,
                  sample_t **allVectors,
                  sample_t eps,
                  vq_learn_callback_t callback
                  ) {

    // to maintain and report codeword and minimum distortion for
    // each training vector against each trained codebook size:
    CodewordAndMinDist *minDists = (CodewordAndMinDist *) calloc(tot_vecs, sizeof(CodewordAndMinDist));

    int pass = 0;

    char cb_filename[2048];
    #pragma GCC diagnostic ignored "-Wformat-overflow"
    sprintf(cb_filename, "%s_M_%04d.cbook", prefix, num_raas);
    printf("%s\n", cb_filename);

    // for global processing time:
    const double measure_start_sec = measure_time_now_sec();

    // for particular codebook size:
    double measure_start_cb_sec = measure_time_now_sec();

    int      cardd[MAX_CODEBOOK_SIZE];   // cardinalities
    sample_t discel[MAX_CODEBOOK_SIZE];  // distortions per cell

    sample_t DDprv = SAMPLE_MAX;
    for (;;) {
        printf("(%d)", pass);
        fflush(stdout);

        init_cells(P, cells, num_raas, cardd, discel);

        sample_t DD = 0;

        for (int v = 0; v < tot_vecs; v++) {
            sample_t *rxg = allVectors[v];
            sample_t ddmin = SAMPLE_MAX;
            int raa_min = -1;

            sample_t *raa = codebook;
            for (int i = 0; i < num_raas; i++, raa += (1 + P)) {
                sample_t dd = distortion(rxg, raa, P);
                if (dd < ddmin) {
                    ddmin = dd;
                    raa_min = i;
                }
            }
            minDists[v].codeword = raa_min;
            minDists[v].minDist = ddmin - 1;

            // the -1 here moved to `DD -= tot_vecs` after the loop
            DD += ddmin;

            add_to_cell(P, rxg, ddmin, cardd, discel, raa_min);
        }

        DD -= tot_vecs;

        const sample_t avgDistortion = DD / tot_vecs;

        printf("\tDP=%g\tDDprv=%g\tDD=%g\t(DDprv-DD)/DD=%g\r",
               avgDistortion, DDprv, DD, ((DDprv - DD) / DD));
        fflush(stdout);

        review_cells(num_raas, cardd);

        calculate_reflections(P, cardd, num_raas);

        if (pass > 0 && ((DDprv - DD) / DD) < eps) {
            // done with this codebook size.
            printf("\n      (%d-cbook) processing took %.2fs\n",
                    num_raas, measure_time_now_sec() - measure_start_cb_sec);

            // codebook saved with reflections
            cb_save(codebook_class_name, reflections, num_raas, P, cb_filename);

            sample_t sigma = calculateSigma(codebook, cells, num_raas, P, avgDistortion);
            sample_t inertia = calculateInertia(allVectors,  tot_vecs, codebook, num_raas, P);

            if (callback != 0) {
                callback(num_raas, avgDistortion, sigma, inertia);
            }

            report_cbook(cb_filename, pass + 1, avgDistortion, sigma, inertia, num_raas, cardd, discel,
                         minDists);

            printf("\n");

            if (num_raas >= MAX_CODEBOOK_SIZE) {
                break;
            }

            pass = 0;
            num_raas = grow_codebook(num_raas, P);
            sprintf(cb_filename, "%s_M_%04d.cbook", prefix, num_raas);
            printf("%s\n", cb_filename);

            measure_start_cb_sec = measure_time_now_sec();
        }
        else {
            pass++;
        }

        DDprv = DD;
    }
    printf("\nprocessing took %.2fs\n", measure_time_now_sec() - measure_start_sec);

    free(minDists);
}

int vq_learn(int prediction_order,
             sample_t epsilon,
             const char *codebook_class_name,
             const char *predictor_filenames[],
             int num_predictors,
             vq_learn_callback_t callback
        ) {

    const int P = prediction_order;
    const sample_t eps = epsilon;

    printf("\nCodebook generation:\n\n");
    printf("P=%d eps=%g  class='%s'\n\n", P, eps, codebook_class_name);

    const char *dir_codebooks = "data/codebooks";
    static char class_dir[2048];
    sprintf(class_dir, "%s/%s", dir_codebooks, codebook_class_name);
    mk_dirs(class_dir);

    // to name report and codebooks
    char prefix[2048];
    #pragma GCC diagnostic ignored "-Wformat-overflow"
    sprintf(prefix, "%s/eps_%g", class_dir, eps);

    init_codebook_and_reflections(P);

    int num_raas = initial_codebook(P);

    // load predictors
    Predictor **predictors = (Predictor **) calloc(num_predictors, sizeof(Predictor*));
    long tot_vecs = 0;
    for (int i = 0; i < num_predictors; i++) {
        const char *prdFilename = predictor_filenames[i];
        predictors[i] = prd_load(prdFilename);
        if (!predictors[i]) {
            fprintf(stderr, "error loading predictor %s\n", prdFilename);
            return 2;
        }
        tot_vecs += predictors[i]->T;
    }
    printf("%ld training vectors (ε=%g)\n", tot_vecs, eps);

    sample_t **allVectors = (sample_t **) calloc(tot_vecs, sizeof(sample_t *));
    int v = 0;
    for (int i = 0; i < num_predictors; i++) {
        for (int t = 0; t < predictors[i]->T; t++) {
            allVectors[v++] = predictors[i]->vectors[t];
        }
    }

    char nom_rpt[2048];
    sprintf(nom_rpt, "%s.rpt", prefix);
    prepare_report(nom_rpt, tot_vecs, eps);

    learn(codebook_class_name,
          P, prefix,
          num_raas, tot_vecs, allVectors, eps, callback);

    free(allVectors);

    for (int i = 0; i < num_predictors; i++) {
        prd_destroy(predictors[i]);
    }
    free(predictors);

    close_report();

    return 0;
}
