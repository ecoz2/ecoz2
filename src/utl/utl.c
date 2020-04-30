/* utl.c
 */

#include "utl.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>  // clock_gettime

int ends_with(char* filename, char* str) {
    const int len_i = strlen(filename);
    const int len_j = strlen(str);
    if (len_i < len_j) {
        return 0;
    }
    const int num_checks = len_j;
    int i = len_i - 1;
    int j = len_j - 1;
    for (int k = 0; k < num_checks; ++k) {
        if (filename[i] != str[j]) {
            return 0;
        }
    }
    return 1;
}

double measure_time_now_sec() {
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (double) t.tv_sec + ((double) t.tv_nsec / 1.0e9);
}
