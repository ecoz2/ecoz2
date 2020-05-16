/* distr.h
*/

#ifndef __ECOZ_DISTR_H
#define __ECOZ_DISTR_H

#include "prob_t.h"

/// Sets a random distribution, with no null-values
void dis_set_random(prob_t *dis, int len);

/// Sets a random left-right distribution from a given position
void dis_set_random_delta(prob_t *dis, int len, int from, int delta);

/// Sets a uniform distribution
void dis_set_uniform(prob_t *dis, int len);

/// Returns an event as outcome of an experiment over the distribution
int dis_event(prob_t *dis, int len);

#endif
