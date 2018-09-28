/* symbol.h -- ECOZ System
 */

#ifndef __ECOZ_SYMBOL_H
#define __ECOZ_SYMBOL_H

typedef unsigned short Symbol;

Symbol *seq_create(int T);

Symbol *seq_load(const char *filename, int *T, int *M, char *className);

int seq_save(Symbol *seq, int T, int M, const char *className, char *filename);

void seq_show(Symbol *seq, int T);

#endif
