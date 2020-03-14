/* sgn.c -- ECOZ System
 */

#include "sgn.h"

#include <stdio.h>
#include <stdlib.h>

#include "dr_wav.h"


Sgn *sgn_load(char *filename) {
    unsigned int channels;
    unsigned int sampleRate;
    drwav_uint64 numSamples;
    float *pSampleData = drwav_open_and_read_file_f32(filename, &channels, &sampleRate, &numSamples);
    if (!pSampleData) {
        fprintf(stderr, "Error loading wave file: %s\n", filename);
        drwav_free(pSampleData);
        return 0;
    }
    if (channels != 1) {
        fprintf(stderr, "Sorry, only one channel expected. %s\n", filename);
        drwav_free(pSampleData);
        return 0;
    }

    //printf("input file: %s\n", filename);
    //printf("  channels:      %d\n", channels);
    //printf("  sample rate:   %u\n", sampleRate);
    //printf("  numSamples:    %llu\n", numSamples);

    sample_t *samples = (sample_t *) malloc(numSamples * sizeof(sample_t));
    if (!samples) {
        fprintf(stderr, "Cannot allocate memory for %s\n", filename);
        drwav_free(pSampleData);
        return 0;
    }

    for (unsigned int j = 0; j < numSamples; j++) {
        samples[j] = pSampleData[j];
    }

    drwav_free(pSampleData);

    Sgn *s = (Sgn *) malloc(sizeof(Sgn));
    s->numSamples = numSamples;
    s->sampleRate = sampleRate;
    s->samples = samples;

    return s;
}

int sgn_save(Sgn *s, char *filename) {
    drwav_data_format format;
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
    format.channels = 1;
    format.sampleRate = s->sampleRate;
    format.bitsPerSample = 32;
    drwav *pWav = drwav_open_file_write(filename, &format);
    if (!pWav) {
        fprintf(stderr, "Error creating wave file: %s\n", filename);
        return 1;
    }

    float floats[s->numSamples];
    for (int i = 0; i < s->numSamples; i++) {
        floats[i] = (float) s->samples[i];
    }
    drwav_uint64 samplesWritten = drwav_write(pWav, s->numSamples, floats);
    if (samplesWritten != (drwav_uint64) s->numSamples) {
        fprintf(stderr, "WARN: samplesWritten=%llu != %d\n", samplesWritten, s->numSamples);
    }

    drwav_close(pWav);
    return 0;
}

void sgn_destroy(Sgn *s) {
    free(s->samples);
    free(s);
}

void sgn_show(Sgn *s) {
    printf("numSamples    = %d\n", s->numSamples);
    printf("sampleRate    = %g\n", s->sampleRate);
}

void sgn_preemphasis(Sgn *s) {
    int numSamples = s->numSamples;

    // x[n]
    double *x_n = s->samples + numSamples - 1;

    // x[n-1]
    double *x_n1 = x_n - 1;

    for (long n = numSamples - 1; n > 0; n--, x_n--, x_n1--) {
        *x_n = *x_n - .95 * *x_n1;
    }
}
