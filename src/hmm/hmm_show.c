/* hmm_show.c -- ECOZ System
 */

#include "hmm.h"

#include <stdio.h>


int hmm_show(char *filename, char *format) {

    Hmm *hmm = hmm_load(filename);
    if (!hmm) {
        printf("%s: error loading model\n", filename);
        return 2;
    }

    hmm_show_model(hmm, format);

    hmm_destroy(hmm);
    return 0;
}
