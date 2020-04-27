/* lpa_on_signal.c -- ECOZ System
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

//#undef PAR
#define PAR 1

#ifdef PAR
    #include <omp.h>
#endif

#include "lpc.h"
#include "utl.h"

// M_PI dropped in C99 (?)
#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

static inline void fill_frame(sample_t *from, int numSamples, sample_t *to) {
    for (int n = 0; n < numSamples; n++, from++, to++) {
        *to = *from;
    }
}

static inline void remove_mean(sample_t *frame, int numSamples) {
    sample_t sum = 0;
    sample_t *s = frame;
    for (int n = 0; n < numSamples; n++, s++) {
        sum += *s;
    }
    const sample_t mean = sum / numSamples;
    s = frame;
    for (int n = 0; n < numSamples; n++, s++) {
        *s -= mean;
    }
}

static inline void preemphasis(sample_t *frame, int numSamples) {
    // x[n]
    sample_t *x_n = frame + numSamples - 1;

    // x[n-1]
    sample_t *x_n1 = x_n - 1;

    for (long n = numSamples - 1; n > 0; n--, x_n--, x_n1--) {
        *x_n = *x_n - .95 * *x_n1;
    }
}

static inline void apply_hamming(sample_t *hamming, sample_t *frame, int numSamples) {
    for (int n = 0; n < numSamples; n++, frame++) {
        *frame *= hamming[n];
    }
}

Predictor *lpa_on_signal(int P, int windowLengthMs, int offsetLengthMs, Sgn *sgn) {
    sample_t *signal = sgn->samples;
    const long numSamples = sgn->numSamples;
    const long sampleRate = sgn->sampleRate;

    // number of samples corresponding to windowLengthMs:
    const int winSize = (int) ((windowLengthMs * sampleRate) / 1000);

    // number of samples corresponding to offsetLengthMs:
    const int offset = (int) ((offsetLengthMs * sampleRate) / 1000);

    if (winSize > numSamples) {
        fprintf(stderr, "ERROR: lpa_on_signal: signal too short\n");
        return 0;
    }

    // total number of frames:
    int T = (int) (numSamples - (winSize - offset)) / offset;
    // discard last section if incomplete:
    if ((T - 1) * offset + winSize > numSamples) {
        T--;
    }

    Predictor *predictor = prd_create(T, P, ""); // "" = unknown className
    if (!predictor) {
        fprintf(stderr, "lpa_on_signal: cannot get predictor object\n");
        return 0;
    }

    printf("lpa_on_signal: P=%d numSamples=%ld sampleRate=%ld winSize=%d offset=%d T=%d\n",
           P, numSamples, sampleRate, winSize, offset, T);

    sample_t hamming[winSize];
    for (int n = 0; n < winSize; n++) {
        hamming[n] = .54 - .46 * cos((n * 2 * M_PI) / (winSize - 1));
    }

#ifdef PAR
    const int desired_threads = omp_get_max_threads();
    omp_set_num_threads(desired_threads);
    printf("(desired_threads=%d", desired_threads);
    #pragma omp parallel
    {
    int id = omp_get_thread_num();
    int actual_threads = omp_get_num_threads();
    if (id == 0) {
        printf(" actual_threads=%d)\n", actual_threads);
    }
    #pragma omp parallel for
    for (int t = id; t < T; t += actual_threads) {
#else
    for (int t = 0; t < T; ++t) {
#endif
        sample_t *samples = signal + t * offset;

        // perform linear prediction to each frame:
        sample_t frame[winSize];

        fill_frame(samples, winSize, frame);
        remove_mean(frame, winSize);
        preemphasis(frame, winSize);
        apply_hamming(hamming, frame, winSize);

        // do LPA:
        sample_t reflex[P + 1];   // reflection coefficients
        sample_t pred[P + 1];     // prediction coefficients
        sample_t errPred;         // prediction error

        sample_t *vector = predictor->vectors[t];
        int res_lpca = lpca(frame, winSize, P, vector, reflex, pred, &errPred);
        if (0 != res_lpca) {
            fprintf(stderr, "ERROR: lpa_on_signal: lpca error = %d\n", res_lpca);
            //break;  error: break statement used with OpenMP for loop
        }
        // normalize autocorrelation sequence by gain:
        if (errPred != 0.) {
            for (int n = 0; n <= P; n++) {
                vector[n] /= errPred;
            }
        }
    }
#ifdef PAR
    }
#endif
    printf("  %d total frames processed\n", T);

    return predictor;
}
