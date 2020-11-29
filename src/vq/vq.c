/* vq.c
 */

#include "vq.h"
#include "lpc.h"
#include "utl.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>


////////////////////////////////////////////////////////////////////
// See below new interface for loading already created codebooks.


// error code in operations:

int cb_errnum = 0;

int cb_save(const char *className,
            sample_t *reflections, int num_vecs, int P, char *filename) {

    FILE *file = fopen(filename, "w");
    if (!file) {
        return cb_errnum = 1;
    }

    if (write_file_ident(file, "<codebook>")) {
        fclose(file);
        return cb_errnum = 1;
    }

    if (write_class_name(file, className)) {
        fclose(file);
        return cb_errnum = 1;
    }

    if (1 != fwrite(&P, sizeof(P), 1, file)) {
        fclose(file);
        return cb_errnum = 1;
    }

    if (1 != fwrite(&num_vecs, sizeof(num_vecs), 1, file)) {
        fclose(file);
        return cb_errnum = 1;
    }

    sample_t *entr = reflections;
    for (int i = 0; i < num_vecs; i++, entr += (1 + P)) {
        if (1 != fwrite(entr, (1 + P) * sizeof(sample_t), 1, file)) {
            fclose(file);
            return cb_errnum = 1;
        }
    }

    fclose(file);
    return cb_errnum = 0;
}

sample_t *cb_load(char *filename, int *P, int *num_vecs, char *className) {

    FILE *file = fopen(filename, "r");
    if (!file) {
        cb_errnum = 1;
        return 0;
    }

    if (read_file_ident(file, "<codebook>")) {
        fclose(file);
        cb_errnum = 2;
        return 0;
    }

    if (read_class_name(file, className)) {
        fclose(file);
        cb_errnum = 2;
        return 0;
    }

    if (1 != fread(P, sizeof(*P), 1, file)) {
        fclose(file);
        cb_errnum = 3;
        return 0;
    }

    if (1 != fread(num_vecs, sizeof(*num_vecs), 1, file)) {
        fclose(file);
        cb_errnum = 3;
        return 0;
    }

    long mem_size = (long) *num_vecs * (1 + *P) * sizeof(sample_t);

    sample_t *reflections = (sample_t *) malloc(mem_size);
    if (!reflections) {
        cb_errnum = 3;
        fclose(file);
        return 0;
    }

    sample_t *vec = reflections;
    for (int i = 0; i < *num_vecs; i++, vec += (1 + *P)) {
        if (1 != fread(vec, (1 + *P) * sizeof(sample_t), 1, file)) {
            fclose(file);
            cb_errnum = 4;
            free(reflections);
            return 0;
        }
    }
    fclose(file);

    cb_errnum = 0;
    return reflections;
}


////////////////////////////////////////////////////////////////////
// new scheme.  Now only for loading already created codebooks


Codebook *cbook_load(char *filename) {
    Codebook *cbook = (Codebook *) calloc(1, sizeof(Codebook));
    if (!cbook)
        return 0;

    cbook->reflections = cb_load(filename, &cbook->P, &cbook->num_vecs, cbook->className);
    if (!cbook->reflections) {
        cbook_destroy(cbook);
        return 0;
    }

    const int P = cbook->P;

    cbook->raas = (sample_t *) calloc(cbook->num_vecs, (1 + P) * sizeof(sample_t));
    if (!cbook->raas) {
        cbook_destroy(cbook);
        return 0;
    }
    reflections_to_raas(cbook->reflections, cbook->raas, cbook->num_vecs, cbook->P);

    return cbook;
}

void cbook_destroy(Codebook *cbook) {
    if (cbook->raas)
        free(cbook->raas);
    if (cbook->reflections)
        free(cbook->reflections);
    free(cbook);
}

sample_t cbook_quantize(Codebook *cbook, Predictor *prd, Symbol *seq) {
    assert(cbook->P == prd->P);
    return quantize(cbook->raas, cbook->num_vecs, prd, seq);
}


///////////////////////////////////////

SeqProvider *seq_provider_create(
        int num_models,
        int Mcmp,

        char **seq_filenames,
        unsigned num_seq_filenames,

        char **cb_filenames,
        int num_codebooks,
        char **prd_filenames,
        int num_predictors
        ) {

    assert(num_models > 0);
    assert((seq_filenames != 0) != (cb_filenames != 0));

    SeqProvider *sp = (SeqProvider *) calloc(1, sizeof(SeqProvider));
    assert(sp);

    memset(&sp->next_seq, 0, sizeof(sp->next_seq));

    sp->Mcmp = Mcmp;

    if (seq_filenames) {
        assert(num_seq_filenames > 0);
        sp->seq_filenames = seq_filenames;
        sp->num_sequences = (int) num_seq_filenames;
    }
    else {
        assert(num_codebooks == num_models);
        assert(num_predictors > 0);
        sp->cb_filenames = cb_filenames;
        sp->num_codebooks = num_codebooks;
        sp->prd_filenames = prd_filenames;
        sp->num_predictors = num_predictors;

        // load codebooks:
        sp->codebooks = (Codebook **) calloc(num_models, sizeof(Codebook *));
        int num_vecs = -1;
        printf("\nLoading codebooks:\n");
        for (int i = 0; i < sp->num_codebooks; ++i) {
            printf("%2d: %s\n", i, sp->cb_filenames[i]);
            sp->codebooks[i] = cbook_load(sp->cb_filenames[i]);
            assert(sp->codebooks[i]);
            if (num_vecs < 0) {
                num_vecs = sp->codebooks[i]->num_vecs;
            }
            else {
                // assert conformity
                assert(num_vecs == sp->codebooks[i]->num_vecs);
            }
        }

        sp->next_seq.sequences = (Symbol **) calloc(num_models, sizeof(Symbol *));
        sp->next_seq.dists = (sample_t *) calloc(num_models, sizeof(sample_t));
    }

    sp->next_index = 0;

    //printf("\nSeqProvider created.\n");
    return sp;
}

int seq_provider_with_direct_sequences(SeqProvider *sp) {
    return sp->seq_filenames != 0;
}

int seq_provider_num_instances(SeqProvider *sp) {
    if (seq_provider_with_direct_sequences(sp)) {
        return sp->num_sequences;
    }
    else {
        return sp->num_predictors;
    }
}

int seq_provider_has_next(SeqProvider *sp) {
    if (sp->seq_filenames) {
        return sp->next_index < sp->num_sequences;
    }
    else {
        return sp->next_index < sp->num_predictors;
    }
}

static void _seq_provider_destroy_next_seq(SeqProvider *sp) {
    if (sp->next_seq.sequence) {
        free(sp->next_seq.sequence);
        sp->next_seq.sequence = 0;
    }

    // release the members of this list, but not the list itself:
    if (sp->next_seq.sequences) {
        for (int r = 0; r < sp->num_codebooks; ++r) {
            if (sp->next_seq.sequences[r]) {
                free(sp->next_seq.sequences[r]);
                sp->next_seq.sequences[r] = 0;
            }
        }
    }
}

static NextSeq *_seq_provider_get_next_direct_sequence(SeqProvider *sp) {
    sp->next_seq.seq_filename = sp->seq_filenames[sp->next_index++];

    int M;
    sp->next_seq.sequence = seq_load(sp->next_seq.seq_filename,
                                     &sp->next_seq.T, &M, sp->next_seq.seq_class_name);

    if (!sp->next_seq.sequence) {
        fprintf(stderr, "%s: error loading sequence.\n", sp->next_seq.seq_filename);
        return 0;
    }

    if (sp->Mcmp != M) {
        fprintf(stderr, "%s: conformity error.\n", sp->next_seq.seq_filename);
        _seq_provider_destroy_next_seq(sp);
        return 0;
    }

    return &sp->next_seq;
}

static NextSeq *_seq_provider_get_next_predictor(SeqProvider *sp) {
    // load next predictor:
    sp->next_seq.prd_filename = sp->prd_filenames[sp->next_index++];
    Predictor *prd = prd_load(sp->next_seq.prd_filename);
    if (!prd) {
        fprintf(stderr, "%s: error loading predictor.\n", sp->next_seq.prd_filename);
        return 0;
    }
    strcpy(sp->next_seq.prd_class_name, prd->className);
    const int T = sp->next_seq.T = prd->T;

    //printf("quantizing %s: class=%s  T=%d\n", sp->next_seq.prd_filename, sp->next_seq.prd_class_name, T);

    // quantization with each of the codebooks:
    for (int i = 0; i < sp->num_codebooks; ++i) {
        sp->next_seq.sequences[i] = seq_create(T);
        assert(sp->next_seq.sequences[i]);

        //printf("  quantizing with codebook: %s\n", sp->cb_filenames[i]);

        sample_t dist = cbook_quantize(sp->codebooks[i], prd, sp->next_seq.sequences[i]);
        //printf("  done: distortion: %f\n", dist);
        sp->next_seq.dists[i] = dist;
    }

    prd_destroy(prd);

    //printf("done quantizing.\n");

    return &sp->next_seq;
}

NextSeq *seq_provider_get_next(SeqProvider *sp) {
    _seq_provider_destroy_next_seq(sp);

    if (sp->seq_filenames) {
        assert (sp->next_index < sp->num_sequences);
        return _seq_provider_get_next_direct_sequence(sp);
    }
    else {
        assert (sp->next_index < sp->num_predictors);
        return _seq_provider_get_next_predictor(sp);
    }
}

void seq_provider_destroy(SeqProvider *sp) {
    _seq_provider_destroy_next_seq(sp);
    if (sp->next_seq.sequences) {
        free(sp->next_seq.sequences);
    }
    if (sp->next_seq.dists) {
        free(sp->next_seq.dists);
    }
    if (sp->codebooks) {
        for (int i = 0; i < sp->num_codebooks; ++i) {
            if (sp->codebooks[i]) {
                free(sp->codebooks[i]);
            }
        }
        free(sp->codebooks);
    }
    free(sp);
}
