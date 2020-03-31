from distutils.core import setup
from distutils.extension import Extension
from Cython.Build import cythonize

ecoz_extension = Extension(
    name="ecozpy",
    sources=["src/py/ecoz.pyx"],
    libraries=["ecoz"],
    library_dirs=["_out/lib"],
    include_dirs=["src/include"]
)
setup(
    name="ecozpy",
    ext_modules=cythonize([ecoz_extension])
)
