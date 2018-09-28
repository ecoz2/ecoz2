/*
 * A simple list manager. Each element is a `void *`.
 * list.c - Implementation.
 * Author: Carlos Rueda
 * Date:   5/25/01
 */
#include "list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    void **elements;
    long size;
    long capacity;
    long increment;
} RListObj, *ListPObj;

List list_create() {
    ListPObj list;

    list = (ListPObj) malloc(sizeof(RListObj));
    if (!list) {
        return 0;
    }
    list->capacity = 512;
    list->increment = 512;
    list->elements = (void **) malloc(list->capacity * sizeof(void *));
    if (!list->elements) {
        free(list);
        return 0;
    }
    list->size = 0;

    return list;
}

static int ensureCapacity(ListPObj list, long minCapacity) {
    if (list->capacity < minCapacity) {
        long nbytes = minCapacity * sizeof(void *);
        void **tmp = (void **) realloc(list->elements, nbytes);
        if (tmp == 0) {
            fprintf(stderr, "No memory in reallocation of list, new capacity=%d  element size=%zu\n",
                    (int) minCapacity, sizeof(void *)
            );
            return 1; // problems
        }
        list->elements = tmp;
        list->capacity = minCapacity;
    }
    return 0;  // OK
}

void list_addElement(List list_id, void *element) {
    ListPObj list = (ListPObj) list_id;

    if (list->size == list->capacity) {
        long new_capacity = list->capacity + list->increment;
        if (0 != ensureCapacity(list, new_capacity)) {
            return;
        }
    }
    list->elements[list->size] = element;
    list->size++;
}

long list_size(List list_id) {
    ListPObj list = (ListPObj) list_id;
    return list->size;
}

void *list_elementAt(List list_id, long index) {
    ListPObj list = (ListPObj) list_id;

    if (index < 0L || index >= list->size) {
        return 0;
    }
    else {
        return list->elements[index];
    }
}

void list_shuffle(List list_id) {
    ListPObj list = (ListPObj) list_id;
    for (long i = 0; i < list->size; i++) {
        long j = i + rand() % (list->size - i);
        void *t = list->elements[i];
        list->elements[i] = list->elements[j];
        list->elements[j] = t;
    }
}

void list_destroy(List list_id) {
    ListPObj list = (ListPObj) list_id;
    free(list->elements);
    free(list);
}
