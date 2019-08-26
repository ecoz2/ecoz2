/* vq.h -- ECOZ System
 */

#ifndef __ECOZ_VQ_H
#define __ECOZ_VQ_H

#include "lpc.h"
#include "symbol.h"


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
sample_t distortion(sample_t *rxg, sample_t *raa, int P);

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


#endif
