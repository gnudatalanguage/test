#ifndef __CONFIG_H__
#define __CONFIG_H__

#define EXEC_PREFIX "@CMAKE_INSTALL_PREFIX@"
#define GDLDATADIR "@CMAKE_INSTALL_PREFIX@@GDL_DATA_DIR@"
#define VERSION "@VERSION@"

#define _CRT_SECURE_NO_WARNINGS

#cmakedefine HAVE_X 1
#cmakedefine HAVE_64BIT_OS 1
#cmakedefine HAVE_DLFCN_H 1
#cmakedefine HAVE_INTTYPES_H 1
#cmakedefine HAVE_LIBCURSES 1
#cmakedefine HAVE_LIBGSL 1
#cmakedefine HAVE_LIBGSLCBLAS 1
#cmakedefine HAVE_LIBNCURSES 1
#cmakedefine HAVE_LIBPLPLOTCXXD 1
#cmakedefine HAVE_LIBREADLINE 1
#cmakedefine HAVE_LIBWXWIDGETS 1
#cmakedefine HAVE_LIBZ 1
#cmakedefine HAVE_LOCALE_H 1
#cmakedefine HAVE_SBRK 1
#cmakedefine HAVE_MALLINFO 1
#cmakedefine HAVE_MALLOC_H 1
#cmakedefine HAVE_MALLOC_MALLOC_H 1
#cmakedefine HAVE_MALLOC_ZONE_STATISTICS 1
#cmakedefine HAVE_MEMORY_H 1
#cmakedefine HAVE_NEXTTOWARD 1
#cmakedefine HAVE_PLPLOT_WIDTH 1
#cmakedefine PLPLOT_PRIVATE_NOT_HIDDEN 1
#cmakedefine PLPLOT_HAS_PLCALLBACK
#cmakedefine HAVE_QHULL 1
#define _WCHAR_H_CPLUSPLUS_98_CONFORMANCE_ 1

#ifndef HAVE_STDINT_H
#cmakedefine HAVE_STDINT_H 1
#endif
#ifndef HAVE_STDLIB_H
#cmakedefine HAVE_STDLIB_H 1
#endif
#ifndef HAVE_STRINGS_H
#cmakedefine HAVE_STRINGS_H 1
#endif
#ifndef HAVE_STRING_H
#cmakedefine HAVE_STRING_H 1
#endif
#ifndef HAVE_SYS_STAT_H
#cmakedefine HAVE_SYS_STAT_H 1
#endif
#ifndef HAVE_SYS_TYPES_H
#cmakedefine HAVE_SYS_TYPES_H 1
#endif
#ifndef HAVE_UNISTD_H
#cmakedefine HAVE_UNISTD_H 1
#endif
#ifndef HAVE_EXT_STDIO_FILEBUF_H
#cmakedefine HAVE_EXT_STDIO_FILEBUF_H 1
#endif
#cmakedefine IS_BIGENDIAN
#cmakedefine OLD_DARWIN
#cmakedefine PYTHON_MODULE 1
#cmakedefine RL_GET_SCREEN_SIZE 1
#cmakedefine STDC_HEADERS 1
#cmakedefine USE_FFTW 1
#cmakedefine USE_GRIB 1
#cmakedefine USE_GLPK 1
#cmakedefine USE_SHAPELIB 1
#cmakedefine USE_EXPAT 1
#cmakedefine USE_HDF 1
#cmakedefine USE_HDF5 1
#cmakedefine USE_LIBPROJ 1
#define LIBPROJ_MAJOR_VERSION @LIBPROJ_MAJOR_VERSION@
#cmakedefine USE_MAGICK 1
#cmakedefine USE_MPI 1
#cmakedefine USE_NETCDF 1
#cmakedefine USE_NETCDF4 1
#cmakedefine USE_OPENMP 1
#cmakedefine USE_PYTHON 1
#cmakedefine USE_TIFF 1
#cmakedefine USE_GEOTIFF 1
#cmakedefine USE_UDUNITS 1
#cmakedefine USE_EIGEN 1
#cmakedefine USE_PNGLIB 1
#cmakedefine USE_WINGDI_NOT_WINGCC 1
#endif
