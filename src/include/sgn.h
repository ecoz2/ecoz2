/* sgn.h -- ECOZ System
 */

#ifndef __ECOZ_SGN_H
#define __ECOZ_SGN_H

#ifdef __cplusplus
extern "C" {
#endif

typedef double sample_t;

#define SAMPLE_MAX DBL_MAX


/**
 * Audio signal representation
 */
typedef struct {
    sample_t *samples;
    int numSamples;
    double sampleRate;
} Sgn;


/**
 * Loads a signal from a wave file.
 * Note, only one channel is considered.
 */
Sgn *sgn_load(char *filename);

int sgn_save(Sgn *s, char *filename);

void sgn_show(Sgn *s);

/**
 * Applies `[1 - .95z-1]` to the signal.
 */
void sgn_preemphasis(Sgn *);

/**
 * Endpoint detection.
 * @param s       Input signal
 * @param start   Index on input signal
 * @return        Resulting signal, or 0.
 */
Sgn *sgn_endpoint(Sgn *s, long *start);

void sgn_destroy(Sgn *s);

#ifdef __cplusplus
}
#endif

#endif
