#include <omp.h>

static void learn_par(const char *codebook_class_name,
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

    const int desired_threads = omp_get_max_threads();
    omp_set_num_threads(desired_threads);
    printf("\n(desired_threads=%d)\n", desired_threads);

    // info per thread:
    int      t_cardd[desired_threads][MAX_CODEBOOK_SIZE];   // cardinalities per thread
    sample_t t_discel[desired_threads][MAX_CODEBOOK_SIZE];  // distortions per cell per thread
    sample_t t_cells[desired_threads][MAX_CODEBOOK_SIZE_IN_VALUES]; // classification on training vectors
    sample_t t_DD[desired_threads];

    // for global processing time:
    const double measure_start_sec = measure_time_now_sec();

    // for particular codebook size:
    double measure_start_cb_sec = measure_time_now_sec();

    sample_t DDprv = SAMPLE_MAX;
    for (;;) {
        printf("(%d)", pass);
        fflush(stdout);

        int max_id = -1;

        double measure_start_min_sec = measure_time_now_sec();

        #pragma omp parallel
        {
            const int id = omp_get_thread_num();
            const int actual_threads = omp_get_num_threads();

            #pragma omp critical
            if (max_id < id) max_id = id;

            init_cells(P, t_cells[id], num_raas, t_cardd[id], t_discel[id]);

            t_DD[id] = 0;

            for (int v = id; v < tot_vecs; v += actual_threads) {
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

                t_DD[id] += ddmin - 1;

                add_to_cell(P, t_cells[id], rxg, ddmin, t_cardd[id], t_discel[id], raa_min);
            }
        }

        //printf("---- max_id=%d\n", max_id);

        //---------------------
        // consolidate thread info:
        int      cardd[MAX_CODEBOOK_SIZE];
        sample_t discel[MAX_CODEBOOK_SIZE];
        for (int i = 0; i < num_raas; i++) {
            cardd[i] = 0;
            discel[i] = 0.f;
            for (int id = 0; id <= max_id; ++id) {
                cardd[i] += t_cardd[id][i];
                discel[i] += t_discel[id][i];
            }
        }

        sample_t DD = 0;
        for (int id = 0; id <= max_id; ++id) {
            DD += t_DD[id];
        }

        sample_t cells[MAX_CODEBOOK_SIZE_IN_VALUES];
        sample_t *cel = cells;
        for (int i = 0; i < (1 + P) * num_raas; ++i, ++cel) {
            *cel = (sample_t) 0.;
            for (int id = 0; id <= max_id; ++id) {
                *cel += t_cells[id][i];
            }
        }
        //---------------------

        printf("\n      (pass=%d) min processing took %.2fs\n",
               pass, measure_time_now_sec() - measure_start_min_sec);

        const sample_t avgDistortion = DD / tot_vecs;

        printf("\tDP=%g\tDDprv=%g\tDD=%g\t(DDprv-DD)/DD=%g\r",
               avgDistortion, DDprv, DD, ((DDprv - DD) / DD));
        fflush(stdout);

        review_cells(num_raas, cardd);

        calculate_reflections(P, cells, cardd, num_raas);

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
