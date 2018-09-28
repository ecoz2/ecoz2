/*
 * A simple list manager. Each element is a `void *`.
 * list.h - Public declarations.
 * Author: Carlos Rueda
 * Date:   5/25/01
 */
#ifndef __LISTP_H
#define __LISTP_H

typedef void *List;

List list_create();

void list_addElement(List list, void *element);

long list_size(List list);

void *list_elementAt(List list, long index);

void list_shuffle(List list);

void list_destroy(List list);

#endif
