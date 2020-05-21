/* vq.h -- ECOZ System
 */

#ifndef __ECOZ_VQ_H
#define __ECOZ_VQ_H

#include "ecoz2.h"
#include "lpc.h"
#include "symbol.h"

#define MAX_CODEBOOK_SIZE 4096

#define MAX_PREDICTION_ORDER 40


#define MAX_CB_MODELS 256


////////////////////////////////////////////////////////////////////
// See new scheme below.


// error code: 
extern int cb_errnum;

int cb_save(const char *className,
            sample_t *reflections, int num_vecs, int P, char *filename
           );

sample_t *cb_load(char *filename, int *P, int *num_vecs, char *className);


////////////////////////////////////////////////////////////////////
// new scheme.  Now only for loading already created codebooks

typedef struct {
    char className[MAX_CLASS_NAME_LEN];
    int P;
    int num_vecs;
    sample_t *raas;
    sample_t *reflections;
} Codebook;

Codebook *cbook_load(char *filename);

sample_t cbook_quantize(Codebook *cbook, Predictor *prd, Symbol *seq);

void cbook_destroy(Codebook *cbook);

/**
  * Likelihood ratio measure as defined in Juang et al (1982) (except no -1 here).
  *
  * @param rx   gain normalized autocorrelation
  * @param ra   prediction coefficient autocorrelation
  * @param P    prediction order
  * @return  distortion
  */
sample_t distortion(const sample_t *rxg, const sample_t *raa, int P);

typedef struct {
    int      codeword;
    sample_t minDist;
} CodewordAndMinDist;

/**
 * Does quantization of the vectors in the given predictor.
 *
 * @param raas        The codebook
 * @param num_raas    Codebook size
 * @param prd         Predictor to quantize
 * @param seq         If not null, resulting symbol sequence is stored here.
 * @return            Average distortion.
 */
sample_t quantize(sample_t *raas,
                  int num_raas,
                  Predictor *prd,
                  Symbol *seq
                 );


/// Clustering starting from codebook size 2
int vq_learn(int prediction_order,
             sample_t epsilon,
             const char *codebook_class_name,
             const char *predictor_filenames[],
             int num_predictors,
             int use_par,
             void* callback_target,
             vq_learn_callback_t callback
            );

/// Clustering starting from next size according to given codebook
int vq_learn_using_base_codebook(
        char *base_codebook,
        sample_t epsilon,
        const char *predictor_filenames[],
        int num_predictors,
        int use_par,
        void* callback_target,
        vq_learn_callback_t callback
        );

int vq_quantize(const char *nom_raas,
                const char *predictor_filenames[],
                int num_predictors,
                int show_filenames
               );

int vq_show(char *codebook_filename, int from, int to);

int vq_classify(
        char **cb_filenames,
        int num_codebooks,
        char **prd_filenames,
        int num_predictors,
        int show_ranked
        );

sample_t calculateSigma(sample_t *codebook, sample_t *cells, int codebookSize, int P, sample_t avgDistortion);

sample_t calculateInertia(sample_t **allVectors, long tot_vecs, sample_t *codebook, int codebookSize, int P);

void prepare_report(char *, long, double);

void report_cbook(char *, int, sample_t, sample_t, sample_t, int, int *, sample_t *, CodewordAndMinDist *);

void close_report(void);

#endif
