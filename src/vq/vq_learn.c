/* vq.learn.c -- ECOZ System
 * Codebook generation according to Juang et al (1982).
 */

#include "vq_learn.i"

int vq_learn(int prediction_order,
             sample_t epsilon,
             const char *codebook_class_name,
             const char *predictor_filenames[],
             int num_predictors,
             int use_par,
             void* callback_target,
             vq_learn_callback_t callback
        ) {

    printf("\nCodebook generation:\n\n");
    printf("prediction_order=%d class='%s'  epsilon=%g\n\n",
            prediction_order, codebook_class_name, epsilon);

    return vq_learn_(
            0, 0,    // no base codebook.
            prediction_order,
            epsilon,
            codebook_class_name,
            predictor_filenames,
            num_predictors,
            use_par,
            callback_target,
            callback
            );
}

int vq_learn_using_base_codebook(
        char *base_codebook,
        sample_t epsilon,
        const char *predictor_filenames[],
        int num_predictors,
        int use_par,
        void* callback_target,
        vq_learn_callback_t callback
                                ) {

    int prediction_order;
    int num_vecs;
    char codebook_class_name[MAX_CLASS_NAME_LEN];

    sample_t *base_reflections = cb_load(base_codebook, &prediction_order, &num_vecs, codebook_class_name);
    if (!base_reflections) {
        fprintf(stderr, "error loading base codebook %s.\n", base_codebook);
        return 2;
    }

    printf("\nCodebook generation:\n\n");
    printf("base_codebook: %s  num_vecs=%d prediction_order=%d class='%s'  epsilon=%g\n\n",
           base_codebook,
           num_vecs,
           prediction_order,
           codebook_class_name, epsilon);

    int result = vq_learn_(
            base_reflections,
            num_vecs,
            prediction_order,
            epsilon,
            codebook_class_name,
            predictor_filenames,
            num_predictors,
            use_par,
            callback_target,
            callback
    );

    free(base_reflections);

    return result;
}
