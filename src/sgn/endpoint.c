/* endpoint.c -- ECOZ System
 * Implementation of Rabiner and Sambur (1975) endpoint detection algorithm.
 */

#include "sgn.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const int debug = 0;

#ifndef min
#define min(x, y) ( (x) < (y) ? (x) : (y) )
#endif

static sample_t *x;        // copy of input signal

static int lonVen;         // longitud de la ventana de 10 ms
static int LMagCeros;      // longitud de las funciones mag[] y cer[]
static sample_t *mag;      // magnitudes promedio de la onda
static sample_t *E;        // mag[] excluyendo los 10 ms iniciales
static int *cer;           // densidad de ceros de la onda
static int *Z;             // cer[] excluyendo los 10 ms iniciales
static sample_t itu, itl;  // umbrales para análisis sobre magnitud
static int izct;           // umbral para análisis sobre rata de ceros
static int n1, n2;         // indican comienzo y final de la palabra
static int n1_, n2_;       // primeros n1 y n2

//static	int	escMag = 5, escCer = 0;

static inline void preemphasis(sample_t *frame, int numSamples) {
    // x[n]
    sample_t *x_n = frame + numSamples - 1;

    // x[n-1]
    sample_t *x_n1 = x_n - 1;

    for (long n = numSamples - 1; n > 0; n--, x_n--, x_n1--) {
        *x_n = *x_n - .95 * *x_n1;
    }
}

/*
	iniciar: asigna memoria para las señales de magnitudes y de densidad de
	ceros. Verifica que la señal a examinar dure por lo menos 2 décimas de
	segundo (la primera décima se asume como ruido de fondo).
*/
static int iniciar(sample_t *unaSenal, long L, long f) {
    if (debug) fprintf(stderr, "EPD: iniciar.  L=%ld  f=%ld\n", L, f);
    if (L < f / 5)
        return 3;

    lonVen = (int) f / 100;
    LMagCeros = (int) L / lonVen - 1;
    if (LMagCeros == 0)
        return -1;

    if (0 == (x = malloc(L * sizeof(sample_t))))
        return -2;

    if (0 == (mag = malloc(LMagCeros * sizeof(sample_t)))) {
        free(x);
        return -3;
    }

    if (0 == (cer = malloc(LMagCeros * sizeof(int)))) {
        free(x);
        free(mag);
        return -4;
    }

    memcpy(x, unaSenal, L * sizeof(sample_t));

    // if maximum value is not >= 4094, scale it:
    double max = x[0];
    double sum = x[0];
    for (int i = 1; i < L; i++) {
        if (max < x[i]) {
            max = x[i];
        }
        sum += x[i];
    }
    if (max < 4096) {
        if (debug) printf("EPD: scaling signal\n");
        for (int i = 1; i < L; i++) {
            x[i] = 4096. * x[i] / max;
        }
    }

    if (debug) printf("END: doing preemphasis\n");
    preemphasis(x, L);

    return 0;
}

static void listo() {
    free(cer);
    free(mag);
    free(x);
}

/*
	obtener_cer: calcula la función de densidad de ceros.
*/
static void obtener_cer(sample_t *x) {
    for (int c = 0, n = 0; c < LMagCeros; c++) {
        cer[c] = 0;
        int mayIg0 = x[n] >= 0;

        /* iterate lonVen-1 times: */
        for (int t = 1; t < lonVen; t++) {
            int mayIg0Sig = x[++n] >= 0;
            if (mayIg0 != mayIg0Sig) {
                cer[c]++;
                mayIg0 = mayIg0Sig;
            }
        }
        n++;
    }
    Z = cer + 10;
}

/*
	obtener_izct: cálculo del umbral izct que se obtiene de los primeros 100
	ms de la la señal x[], i.e. de los 10 primeros valores de cer[].
*/
static void obtener_izct() {
    double media;        // media of cer[0:9]
    double desv;         // standard deviation of cer[0:9]
    long ceros2;         // para desviación estándar, parte �(cer[c])�

    media = 0.;
    ceros2 = 0L;
    for (int c = 0; c < 10; c++) {
        media += (double) cer[c];
        ceros2 += (long) cer[c] * cer[c];
    }
    double radical = (double) ((double) ceros2 - media * media / 10) / 10;
    desv = radical > (double) 0. ? sqrt(radical) : (double) 0.;
    media /= 10;

    int altern = round(media + 2 * desv);

    if (altern < 9) altern = 9;

    izct = min(25, altern);
}

/*
	obtener_mag: calcula la función de magnitudes.
*/
static void obtener_mag(sample_t *x) {
    for (int m = 0, n = 0; m < LMagCeros; m++) {
        mag[m] = 0;
        for (int t = 0; t < lonVen; t++, n++) {
            mag[m] += fabs(x[n]);
        }
    }
    E = mag + 10;
}

/*
	obtener_itl_itu: cálculo de los umbrales relativos a magnitudes.
*/
static void obtener_itl_itu() {
    double media;      // media de mag[0:9]
    double desv;       // desviaci�n est�ndar de mag[0:9]
    double imn;        // energ�a del silencio
    double radical;
    sample_t imx;        // máximo valor de mag[10:LMagCeros]
    int mMax;          // mag[mMax] == maxMag
    double mag2;       // para desviación estándar, parte �(mag[m])�
    int m;
    long i1, i2;

    // análisis del "silencio":

    media = 0.;
    mag2 = 0.;
    for (m = 0; m < 10; m++) {
        media += (double) mag[m];
        mag2 += mag[m] * mag[m];
    }
    radical = (mag2 - (media * media) / 10) / 10;
    desv = radical > (double) 0. ? sqrt(radical) : (double) 0.;
    media /= 10;

    imn = media + 2 * desv;            // intuitivo
    if (imn < (double) 12.) imn = (double) 12.;

    // encontrar máxima magnitud del resto de la señal:
    mMax = 10;
    for (m = 11; m < LMagCeros; m++) {
        if (mag[mMax] < mag[m]) {
            mMax = m;
        }
    }
    imx = mag[mMax];

    i1 = (long) round(.03 * (imx - imn) + imn);
    i2 = (long) round(4 * imn);

    itl = i1 < i2 ? i1 : i2;
    itu = 5 * itl;
}

/*
	primer_n1: estimación del primer tramo probable para comienzo.
*/
static int primer_n1() {
    n1 = -100;
    int m = 0;
    g1:
    for (; E[m] < itl && m < LMagCeros - 10; m++);
    if (m >= LMagCeros - 10) {
        if (debug) printf("EPD: m=%d LMagCeros=%d\n", m, LMagCeros);
        return n1;
    }

    int i = m;
    g2:
    if (i >= LMagCeros - 10) {
        if (debug) printf("EPD: i=%d LMagCeros=%d\n", i, LMagCeros);
        return n1;
    }

    if (E[i] < itl) {
        m = i + 1;
        goto g1;
    }
    if (E[i] < itu) {
        i++;
        goto g2;
    }
    n1 = m;
    if (i == m && n1 > 0) n1--;

    return n1;
}

/*
	primer_n2: estimación del último tramo probable para final.
*/
static int primer_n2() {
    int m, i;

    n2 = -100;
    m = LMagCeros - 11;
    g1:
    for (; E[m] < itl && m > 0; m--);
    if (m <= 0)
        return n2;

    i = m;
    g2:
    if (i <= 0)
        return n2;

    if (E[i] < itl) {
        m = i - 1;
        goto g1;
    }
    if (E[i] < itu) {
        i--;
        goto g2;
    }
    n2 = m;
    if (i == m && n2 < LMagCeros - 12) n2++;

    return n2;
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	refinar_n1: encontrar mejor primer tramo para comienzo.
*/
static void refinar_n1() {
    int m, cuantos, hasta, prim = 0;

    hasta = n1 > 25 ? n1 - 25 : 0;
    cuantos = 0;
    for (m = n1; m >= hasta; m--) {
        if (Z[m] >= izct) {
            cuantos++;
            prim = m;
        }
    }
    if (cuantos >= 3)
        n1 = prim;
}

/*
	refinar_n2: encontrar mejor último tramo para final.
*/
static void refinar_n2() {
    int m, cuantos, hasta, prim = 0;

    hasta = n2 < LMagCeros - 10 - 25 ? n2 + 25 : LMagCeros - 10 - 1;
    cuantos = 0;
    for (m = n2; m <= hasta; m++) {
        if (Z[m] >= izct) {
            cuantos++;
            prim = m;
        }
    }
    if (cuantos >= 3)
        n2 = prim;
}

static sample_t *do_endpoint(sample_t *s, long numSamples, long f, long *start, long *L) {
    int status = iniciar(s, numSamples, f);
    if (status) {
        fprintf(stderr, "EPD: status = %d\n", status);
        return 0;
    }

    obtener_cer(x);
    obtener_mag(x);

    obtener_izct();
    obtener_itl_itu();
    if (-100 == primer_n1()) {
        listo();
        if (debug) fprintf(stderr, "EPD: n1 problem\n");
        return 0;
    }
    if (-100 == primer_n2()) {
        listo();
        if (debug) fprintf(stderr, "EPD: n2 problem\n");
        return 0;
    }
    n1_ = n1;
    n2_ = n2;

    refinar_n1();
    refinar_n2();

    listo();
    *L = (long) lonVen * (n2 - n1);

    *start = lonVen * (n1 + 10);
    return s + *start;
}

Sgn *sgn_endpoint(Sgn *s, long *start) {
    long L;
    sample_t *d = do_endpoint(s->samples, s->numSamples,
                              (long) s->sampleRate, start, &L);
    if (!d) {
        return 0;
    }

    Sgn *e = (Sgn *) malloc(sizeof(Sgn));
    e->samples = (sample_t *) malloc(L * sizeof(sample_t));
    e->numSamples = L;
    e->sampleRate = s->sampleRate;

    for (int i = 0; i < L; i++) {
        e->samples[i] = s->samples[*start + i];
    }

    const double percent = 100. * e->numSamples / s->numSamples;
    printf("  start=%ld len=%d (%.1f%% of original %d)\n",
           *start, e->numSamples, percent, s->numSamples);

    return e;
}
