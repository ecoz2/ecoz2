/* lpca.c -- ECOZ System
 * Linear Prediction Analysis (Moore, 1990; Parsons, 1987)
 */

#include "lpc.h"

#include <float.h>

int lpca(sample_t *x, int N, int P, sample_t *r, sample_t *rc, sample_t *a, sample_t *pe) {
    sample_t r0;            /* potencia de la señal */
    sample_t akk;            /* coeficiente de reflexión más reciente */
    int i, k;
    sample_t sum, ai, aj;

    /* computar autocorrelaciones: */
    for (i = 0; i <= P; i++) {
        sum = (sample_t) 0.;
        for (k = 0; k < N - i; k++)
            sum += x[k] * x[k + i];
        r[i] = sum;
    }
    r0 = r[0];
    if ((sample_t) 0. == r0)
        return 1;

    /* computar coeficiente de reflexión y coeficientes de predicción: */
    *pe = r0;
    a[0] = (sample_t) 1.;
    for (k = 1; k <= P; k++) {    /* nuevo coeficiente de reflexión: */
        sum = (sample_t) 0.;
        for (i = 1; i <= k; i++)
            sum -= a[k - i] * r[i];
        akk = sum / *pe;
        rc[k] = akk;

        /* nuevos coeficientes predictivos: */
        a[k] = akk;
        for (i = 1; i <= k >> 1; i++) {
            ai = a[i];
            aj = a[k - i];
            a[i] = ai + akk * aj;
            a[k - i] = aj + akk * ai;
        }
        /* nuevo error de predicción: */
        *pe *= (sample_t)(1. - akk * akk);
        if (*pe <= 0.)
            return 2;
    }
    /**************PRUEBA: predecir la señal:
      for ( k = N-1; k >= P; k-- )
      {	sum = 0.;
          for ( i = 1; i <= P; i++ )
              sum -= a[i] * x[k-i];
          x[k] = sum;
      }
    *****************************************/

    return 0;
}

/*
	lpca_r: análsis de predicción tomando autocorrelaciones.
*/
int lpca_r(int P, sample_t *r, sample_t *rc, sample_t *a, sample_t *pe) {
    sample_t r0;            /* potencia de la señal */
    sample_t akk;            /* coeficiente de reflexión más reciente */
    int i, k;
    sample_t sum, ai, aj;

    r0 = r[0];
    if ((sample_t) 0. == r0) {
        for (i = 0; i <= P; i++)
            *rc++ = *a++ = (sample_t) 0;
        return 1;
    }

    /* computar coeficiente de reflexión y coeficientes de predicción: */
    *pe = r0;
    a[0] = (sample_t) 1.;
    for (k = 1; k <= P; k++) {    /* nuevo coeficiente de reflexión: */
        sum = (sample_t) 0.;
        for (i = 1; i <= k; i++)
            sum -= a[k - i] * r[i];
        akk = sum / *pe;
        rc[k] = akk;

        /* nuevos coeficientes predictivos: */
        a[k] = akk;
        for (i = 1; i <= k >> 1; i++) {
            ai = a[i];
            aj = a[k - i];
            a[i] = ai + akk * aj;
            a[k - i] = aj + akk * ai;
        }
        /* nuevo error de predicción: */
        *pe *= (sample_t)(1. - akk * akk);
        if (*pe <= 0.)
            return 2;
    }
    return 0;
}

/*
	lpca_rc: análisis de predicción tomando coeficientes de reflexión.
*/
int lpca_rc(int P, sample_t *rc, sample_t *a) {
    sample_t akk;            /* coeficiente de reflexión más reciente */
    int i, k;
    sample_t ai, aj;

    a[0] = (sample_t) 1.;
    for (k = 1; k <= P; k++) {    /* coeficiente de reflexión: */
        akk = rc[k];

        /* nuevos coeficientes predictivos: */
        a[k] = akk;
        for (i = 1; i <= k >> 1; i++) {
            ai = a[i];
            aj = a[k - i];
            a[i] = ai + akk * aj;
            a[k - i] = aj + akk * ai;
        }
    }
    return 0;
}
