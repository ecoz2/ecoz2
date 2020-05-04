/* utl.h -- ECOZ System
 */

#ifndef __ECOZ_UTL_H
#define __ECOZ_UTL_H

#include <stdio.h>

#define GREEN(s) "\x1b[32m" s "\x1b[0m"
#define RED(s)   "\x1b[31m" s "\x1b[0m"

#define FILE_IDENT_LEN 16

#define MAX_CLASS_NAME_LEN 96


int ends_with(char* filename, char* str);

double measure_time_now_sec(void);

const char* measure_time_show_elapsed(double elapsed_secs);

int write_file_ident(FILE *file, const char *ident);

int read_file_ident(FILE *file, const char *ident);

int write_class_name(FILE *file, const char *className);

int read_class_name(FILE *file, char *string);

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	camext: pone nueva extensi�n al nombre de archivo indicado. La nueva
	extensi�n puede ser "", o llevar o no llevar el '.' inicial.
*/
void camext(char *nomar, const char *nuevext);

/**
 * Gets the class name according to the given path.
 * This is basically the simple name of the parent directory.
 * Eg., with `path = "foo/bar/baz.ext'` then `className = "bar"`.
 */
void get_class_name(const char *path, char *className, const int classNameLen);

void get_output_filename(char *from, const char *base_dir, const char *ext, char *output);

int mk_dir(const char *path);

int mk_dirs(const char *path);

/**
 * Creates an array of zero values.
 * Release it with `del_vector`.
 */
void *new_vector(int numvals, int tamval);

void _del_vector(void *v);

#define del_vector(v) _del_vector((void*) (v))

/**
 * Creates a numRows x numCols matrix of zero values.
 * Release it with `del_matrix`.
 */
void **new_matrix(int numRows, int numcols, int tamval);

void _del_matrix(void **m);

#define del_matrix(m) _del_matrix((void**) (m))

#endif
