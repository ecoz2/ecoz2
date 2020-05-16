/* distr.c -- ECOZ System
 */

#include "distr.h"

#include <stdlib.h>

#ifndef min
#define min(x, y) ( (x) < (y) ? (x) : (y) )
#endif


static inline prob_t randomValue() {
    return (prob_t) rand() / RAND_MAX;
}

// Random distribution, with no null-values
void dis_set_random(prob_t *dis, int len) {
    prob_t cumulative = 0.;
    for (int i = 0; i < len; i++) {
        cumulative += dis[i] = randomValue() + 1;
    }

    // normalize
    for (int i = 0; i < len; i++) {
        dis[i] /= cumulative;
    }
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	dis_set_random_delta: genera una distribución aleatoria tipo izq-der con
	desplazamiento máximo delta.
*/
void dis_set_random_delta(prob_t *dis, int len, int from, int delta) {
    int i, resto, numval;

    /* poner en ceros toda la distribución: */
    for (i = 0; i < len; i++)
        dis[i] = 0.;

    /* la asignación de valores no negativos comienza en el índice from,
     * desde donde hay len-from valores hasta el final de la distribución: */

    resto = len - from;

    /* número válido para la asignación: */
    numval = min(resto, delta);

    /* generar distribución aleatoria para numval valores: */
    dis_set_random(dis + from, numval);
}

// uniform distribution
void dis_set_uniform(prob_t *dis, int len) {
    prob_t val = (prob_t) 1. / len;

    prob_t cumulative = 0.;
    for (int i = 0; i < len - 1; i++) {
        cumulative += dis[i] = val;
    }

    dis[len - 1] = 1 - cumulative;
}

// returns an event as outcome of an experiment over the distribution
int dis_event(prob_t *dis, int len) {
    prob_t r = randomValue();

    int e = 0;
    while ((r > dis[e]) && (e < len)) {
        r -= dis[e++];
    }

    return e < len ? e : --len;
}
