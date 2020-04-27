/* fileutil.c
 */

#include "utl.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

static const int debug = 0;

void get_class_name(const char *path, char *className, const int classNameLen) {
    *className = 0;

    char tmp[strlen(path) + 1];
    strcpy(tmp, path);

    char *slash = strrchr(tmp, '/');
    if (slash) {
        *slash = 0;
        slash = strrchr(tmp, '/');
        if (slash) {
            strncpy(className, slash + 1, classNameLen - 1);
        }
    }
    className[classNameLen - 1] = 0;
}

void get_output_filename(char *from, const char *base_dir, const char *ext, char *output) {
    char className[2048] = "_";
    char simple[2048];

    char tmp[2048];
    strcpy(tmp, from);
    camext(tmp, ext);

    char *slash = strrchr(tmp, '/');
    if (slash) {
        strcpy(simple, slash + 1);
        *slash = 0;
        slash = strrchr(tmp, '/');
        if (slash) {
            strcpy(className, slash + 1);
        }
    }
    else {
        strcpy(simple, tmp);
    }
    char classDir[2048];
    #pragma GCC diagnostic ignored "-Wformat-overflow"
    sprintf(classDir, "data/%s/%s", base_dir, className);
    int res = mk_dirs(classDir);
    if (res) {
        printf("ERROR creating directory %s: %d: %s\n", classDir, res, strerror(errno));
    }

    sprintf(output, "%s/%s", classDir, simple);
    if (debug) printf("get_output_filename: %s -> %s\n", from, output);
}

int mk_dir(const char *path) {
    if (debug) printf("mk_dir '%s'\n", path);
    int res = mkdir(path, S_IRWXU);
    if (res) {
        if (errno == EEXIST) {
            return 0; // ok
        }
        printf("ERROR creating directory %s: %d: %s\n", path, res, strerror(errno));
    }
    return res;
}

int mk_dirs(const char *path) {
    if (debug) printf("mk_dirs '%s'\n", path);
    char tmp[strlen(path) + 1];
    strcpy(tmp, path);
    char *slash = strchr(tmp, '/');
    while (slash) {
        *slash = 0;
        int res = mk_dir(tmp);
        if (res) {
            return res;
        }
        *slash = '/';
        slash = strchr(slash + 1, '/');
    }
    int res = mk_dir(tmp);
    return res;
}

void camext(char *nomar, const char *nueva_ext) {
    int lon = strlen(nomar);
    int lon_ne = strlen(nueva_ext);

    int i = lon - 1;
    for (; i >= 0; i--)
        if (nomar[i] == '.')
            break;

    if (i < 0)
        return;

    if (lon - i < 3) {
        strcat(nomar, nueva_ext);
        return;
    }

    /* verificar si la nueva extensi�n ser� nula: */
    if (lon_ne == 0 || (lon_ne == 1 && nueva_ext[0] == '.'))
        nomar[i] = 0;
    else if (nueva_ext[0] == '.')
        strcpy(nomar + i, nueva_ext);
    else {
        nomar[i] = '.';
        strcpy(nomar + i + 1, nueva_ext);
    }
}

int write_file_ident(FILE *file, const char *ident) {
    int len = strlen(ident);
    if (len >= FILE_IDENT_LEN) {
        printf("WARN: write_file_ident: len of '%s' >= FILE_IDENT_LEN=%d\n",
               ident, FILE_IDENT_LEN);
        len = FILE_IDENT_LEN - 1;
    }

    char chunk[FILE_IDENT_LEN];
    memcpy(chunk, ident, len);
    for (int i = len; i < FILE_IDENT_LEN; i++) {
        chunk[i] = 0;
    }
    if (1 == fwrite(chunk, sizeof(chunk), 1, file)) {
        return 0;
    }
    else {
        return 2;
    }
}

int read_file_ident(FILE *file, const char *ident) {
    int len = strlen(ident);
    if (len >= FILE_IDENT_LEN) {
        printf("WARN: read_file_ident: len of '%s' >= FILE_IDENT_LEN=%d\n",
               ident, FILE_IDENT_LEN);
        len = FILE_IDENT_LEN - 1;
    }

    char chunk[FILE_IDENT_LEN];
    if (1 != fread(chunk, sizeof(chunk), 1, file)) {
        printf("WARN: read_file_ident: cannot read %zu bytes\n", sizeof(chunk));
        return -1;
    }
    if (strncmp(ident, chunk, len)) {
        // printf("WARN: read_file_ident: file is not '%s'\n", ident);
        return -2;
    }
    return 0;
}

static int write_fixed_size_string(FILE *file, const char *string, const int fixedLen) {
    int len = strlen(string);
    if (len >= fixedLen) {
        printf("WARN: write_fixed_size_string: len of '%s' >= %d\n",
               string, fixedLen);
        len = fixedLen - 1;
    }

    char chunk[fixedLen];
    memcpy(chunk, string, len);
    chunk[len] = 0;
    for (int i = len + 1; i < fixedLen; i++) {
        chunk[i] = '_';
    }
    if (1 == fwrite(chunk, fixedLen, 1, file)) {
        return 0;
    }
    else {
        return 1;
    }
}

static int read_fixed_size_string(FILE *file, char *string, const int fixedLen) {
    if (1 != fread(string, fixedLen, 1, file)) {
        printf("WARN: read_fixed_size_string: cannot read %d bytes\n", fixedLen);
        return -1;
    }
    string[fixedLen - 1] = 0;
    return 0;
}

int write_class_name(FILE *file, const char *className) {
    return write_fixed_size_string(file, className, MAX_CLASS_NAME_LEN);
}

int read_class_name(FILE *file, char *className) {
    return read_fixed_size_string(file, className, MAX_CLASS_NAME_LEN);
}
