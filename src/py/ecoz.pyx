cdef extern from "ecoz.h":
    const char *ecoz_version()
    const char *ecoz_foo(const char *name)

def version() -> bytes:
    return ecoz_version()

def foo(name: bytes) -> bytes:
    return ecoz_foo(name)
