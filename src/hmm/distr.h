/* distr.h
*/

#ifndef __ECOZ_DISTR_H
#define __ECOZ_DISTR_H

#include "prob_t.h"

void dis_inicAle(prob_t *dis, int lon);

void dis_inicDelta(prob_t *dis, int lon, int desde, int delta);

void dis_inicUni(prob_t *dis, int lon);

int dis_event(prob_t *dis, int lon);

#endif
