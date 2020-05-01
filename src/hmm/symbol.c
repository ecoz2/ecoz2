/* symbol.c -- ECOZ System
 */

#include "symbol.h"
#include "utl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


Symbol *seq_create(int T) {
    return (Symbol *) malloc(T * sizeof(Symbol));
}

int seq_save(Symbol *seq, int T, int M, const char *className, char *filename) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        return 1;
    }
    if (T <= 0) {
        printf("WARN: seq_save: T=%d <= 0: %s\n", T, filename);
    }

    int ret = 0;
    if (write_file_ident(file, "<sequence>")
        || write_class_name(file, className)
        || 1 != fwrite(&T, sizeof(T), 1, file)
        || 1 != fwrite(&M, sizeof(M), 1, file)
        || 1 != fwrite(seq, T * sizeof(Symbol), 1, file))
        ret = 2;

    fclose(file);
    return ret;
}

Symbol *seq_load(const char *filename, int *T, int *M, char *className) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        return 0;
    }

    Symbol *seq = 0;

    if (read_file_ident(file, "<sequence>")) {
        printf("%s: not a sequence\n", filename);
        goto done;
    }

    if (read_class_name(file, className)) {
        printf("%s: could not read className\n", filename);
        goto done;
    }
    // printf("%s: className='%s'\n", filename, className);

    if (1 != fread(T, sizeof(*T), 1, file)) {
        printf("%s: could not read T\n", filename);
        goto done;
    }
    // printf("%s: T=%d\n", filename, *T);

    if (1 != fread(M, sizeof(*M), 1, file)) {
        printf("%s: could not read M\n", filename);
        goto done;
    }
    // printf("%s: M=%d\n", filename, *M);

    seq = seq_create(*T);
    if (!seq) {
        printf("%s: could not created sequence\n", filename);
        goto done;
    }

    int code = fread(seq, *T * sizeof(Symbol), 1, file);
    if (1 != code) {
        printf("error reading %d symbols: code=%d, %s\n", *T, code, strerror(errno));
        free(seq);
        seq = 0;
    }

    done:
    fclose(file);

    return seq;
}

void seq_show(Symbol *O, int T) {
    const int lim = 40;
    const int mrg = 10;
    printf("<(%d): ", T);
    char *comma = "";
    for (int t = 0; t < T; t++) {
        if (T > lim && t >= mrg && t < T - mrg) {
            printf(", ...");
            t = T - mrg;
        }
        printf("%s%d", comma, O[t]);
        comma = ", ";
    }
    printf(">\n");
}
