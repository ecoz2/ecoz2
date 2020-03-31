#include "ecoz.h"

#include <string.h>

const char *ecoz_version() {
    return "ECOZ 2";
}

static char ecoz_foo_result[256];

const char *ecoz_foo(const char *name) {
    const char *prefix = "Hi ";
    strcpy(ecoz_foo_result, prefix);
    strncat(ecoz_foo_result, name, sizeof(ecoz_foo_result) - 10);
    return ecoz_foo_result;
}

int ecoz_baz() {
    return 142857;
}
