/* distr.c -- ECOZ System
 */

#include "distr.h"

#include <stdlib.h>
#include <time.h>

#ifndef min
#define min(x, y) ( (x) < (y) ? (x) : (y) )
#endif


// to control initialization of random value generation:

static int do_randomize = 1;
static int randomize_inited = 0;

static prob_t randomValue() {
    if (!randomize_inited) {
        randomize_inited = 1;
        if (do_randomize) {
            srand(clock());
        }
    }
    return (prob_t) rand() / RAND_MAX;
}

// Random distribution, with no null-values
void dis_inicAle(prob_t *dis, int lon) {
    prob_t acum = 0.;
    for (int i = 0; i < lon; i++) {
        acum += dis[i] = randomValue() + 1;
    }

    // normalize
    for (int i = 0; i < lon; i++) {
        dis[i] /= acum;
    }
}

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	dis_inicDelta: genera una distribución aleatoria tipo izq-der con
	desplazamiento máximo delta.
*/
void dis_inicDelta(prob_t *dis, int lon, int desde, int delta) {
    int i, resto, numval;

    /* poner en ceros toda la distribución: */
    for (i = 0; i < lon; i++)
        dis[i] = 0.;

    /* la asignación de valores no negativos comienza en el índice desde,
     * desde donde hay lon-desde valores hasta el final de la distribución: */

    resto = lon - desde;

    /* número válido para la asignación: */
    numval = min(resto, delta);

    /* generar distribución aleatoria para numval valores: */
    dis_inicAle(dis + desde, numval);
}

// uniform distribution
void dis_inicUni(prob_t *dis, int lon) {
    prob_t val = (prob_t) 1. / lon;

    prob_t acum = 0.;
    for (int i = 0; i < lon - 1; i++) {
        acum += dis[i] = val;
    }

    // el acumulado es por cuestiones de precisión.

    dis[lon - 1] = 1 - acum;
}

// returns an event as outcome of an experiment over the distribution
int dis_event(prob_t *dis, int ldis) {
    prob_t r = randomValue();

    int e = 0;
    while ((r > dis[e]) && (e < ldis)) {
        r -= dis[e++];
    }

    return e < ldis ? e : --ldis;
}
