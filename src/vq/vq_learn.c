/* vq.learn.c -- ECOZ System
 * Codebook generation according to Juang et al (1982).
 */

#include "vq_learn.i"

#include "vq_learn_par.c"
#include "vq_learn_ser.c"

int vq_learn(int prediction_order,
             sample_t epsilon,
             const char *codebook_class_name,
             const char *predictor_filenames[],
             int num_predictors,
             int use_par,
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

    if (use_par) {
        learn_par(codebook_class_name,
                  P, prefix,
                  num_raas, tot_vecs, allVectors, eps, callback);

    }
    else {
        printf("(using serialized impl)\n");
        learn_ser(codebook_class_name,
              P, prefix,
              num_raas, tot_vecs, allVectors, eps, callback);
    }


    free(allVectors);

    for (int i = 0; i < num_predictors; i++) {
        prd_destroy(predictors[i]);
    }
    free(predictors);

    close_report();

    return 0;
}
