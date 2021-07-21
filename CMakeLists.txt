#
# copyright : (c) 2010 Maxime Lenoir and Alain Coulais
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#

cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

# requirement for CMAKE_CXX_STANDARD (3.2 for Apple, 3.1 for non-Apple: https://public.kitware.com/Bug/bug_relationship_graph.php?bug_id=15355)
if (${CMAKE_VERSION} VERSION_LESS "3.2")
  string(FIND "${CMAKE_CXX_FLAGS}" "-std=c++11" tmp)
  if(tmp LESS 0)
    message(FATAL_ERROR "please upgrade CMake to a version 3.2 or higher OR rerun cmake with -DCMAKE_CXX_FLAGS=\"-std=c++11\"")
  endif()
else()
  set(CMAKE_CXX_STANDARD 11)
endif()

cmake_policy(SET CMP0026 OLD) # LOCATION property

project(GDL)
set(CMAKE_CXX_STANDARD 11)

# advice by Orion, mandatory for FC 28
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# X.X.X git becomes release X.X.X+1
set(VERSION "1.0.0-rc.3 git")
enable_testing()

# cf issue 511, managing MultiArch on Debian ...
include(GNUInstallDirs)

include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckLibraryExists)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckCSourceRuns)
include(CheckCXXSourceCompiles)
include(FindPkgConfig)
include(FindPackageHandleStandardArgs)
include(TestBigEndian)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/CMakeModules)
#### GDL is an interactive tool. Normally INTERACTIVE_GRAPHICS must be set
set(INTERACTIVE_GRAPHICS ON CACHE BOOL "GDL: Disable check of existence of a suited graphics driver in plplot.)")

#### Default cached values
set(GDLDEV OFF CACHE BOOL "GDL: Enable GDL developer mode (includes work-in-progress tests)")

set(PYTHON_MODULE OFF CACHE BOOL "GDL: Build GDL as a Python module ?")

set(X11 OFF CACHE BOOL "GDL: Enable X11 ?") 
set(X11DIR "" CACHE PATH "GDL: Specify X11 directory tree")

# Only GNU Readline since May 2020 (too complex to manage BSD Editline. See #754)
set(READLINE ON CACHE BOOL "GDL: Enable GNU Readline ?")
set(READLINEDIR "" CACHE PATH "GDL: Specify the GNU Readline directory tree")

set(PLPLOTDIR "" CACHE PATH "GDL: Specify the plplot directory tree")

set(GSLDIR "" CACHE PATH "GDL: Specify the GSL directory tree")

set(ZLIBDIR "" CACHE PATH "GDL: Specify the Zlib directory tree")

set(WXWIDGETS ON CACHE BOOL "GDL: Enable WxWidgets ?")
set(WXWIDGETSDIR "" CACHE PATH "GDL: Specify WxWidgets directory tree")

set(UDUNITS2 ON CACHE BOOL "GDL: Enable UDUNITS-2 ?")
set(UDUNITS2DIR "" CACHE PATH "GDL: Specify the UDUNITS-2 directory tree")

set(EIGEN3 ON CACHE BOOL "GDL: Enable Eigen3 ?")
set(EIGEN3DIR "" CACHE PATH "GDL: Specify the Eigen3 directory tree")

set(PNGLIB ON CACHE BOOL "GDL: Enable libpng ?")
set(PNGLIBDIR "" CACHE PATH "GDL: Specify the libpng directory tree")

set(GRIB ON CACHE BOOL "GDL: Enable GRIB ?")
set(GRIBDIR "" CACHE PATH "GDL: Specify the ECMWF ecCodes directory tree")

set(QHULL ON CACHE BOOL "GDL: Enable Qhull ?")
set(QHULLDIR "" CACHE PATH "GDL: Specify the QHULL directory tree")

set(GLPK ON CACHE BOOL "GDL: Enable GLPK (for function SIMPLEX) ?")
set(GLPKDIR "" CACHE PATH "GDL: Specify the GLPK directory tree")

# GraphicsMagick is a good alternative to ImageMagick, if GM OK, we don't look for IM
set(GRAPHICSMAGICK ON CACHE BOOL "GDL: Enable GraphicsMagick ?")
set(GRAPHICSMAGICKDIR "" CACHE PATH "GDL: Specify the GraphicsMagick directory tree")

set(MAGICK ON CACHE BOOL "GDL: Enable ImageMagick ?")
set(MAGICKDIR "" CACHE PATH "GDL: Specify the ImageMagick directory tree")

set(TIFF ON CACHE BOOL "GDL: Enable libtiff ?")
set(TIFFDIR "" CACHE PATH "GDL: Specify the libtiff directory tree")

if(TIFF)
    set(GEOTIFF ON CACHE BOOL "GDL: Enable libgeotiff ?")
    set(GEOTIFFDIR "" CACHE PATH "GDL: Specify the libgeotiff directory tree")
endif(TIFF)

set(NETCDF ON CACHE BOOL "GDL: Enable NetCDF ?")
set(NETCDFDIR "" CACHE PATH "GDL: Specify the netCDF directory tree")

set(HDF ON CACHE BOOL "GDL: Enable HDF ?")
set(HDFDIR "" CACHE PATH "GDL: Specify the HDF directory tree")

set(HDF5 ON CACHE BOOL "GDL: Enable HDF5 ?")
set(HDF5DIR "" CACHE PATH "GDL: Specify the HDF5 directory tree")

set(FFTW ON CACHE BOOL "GDL: Enable FFTW ?")
set(FFTWDIR "" CACHE PATH "GDL: Specify the FFTW directory tree")

set(LIBPROJ ON CACHE BOOL "GDL: Enable PROJ ?")
set(LIBPROJDIR "" CACHE PATH "GDL: Specifiy the PROJ directory tree")

set(MPI OFF CACHE BOOL "GDL: Enable MPI ?")
set(MPIDIR "" CACHE PATH "GDL: Specify the MPI (experimental) directory tree")

set(PYTHON ON CACHE BOOL "GDL: Enable Python ?")
set(PYTHONDIR "" CACHE PATH "GDL: Specify the use Python directory tree")
set(PYTHONVERSION "" CACHE STRING "GDL: Specify the Python version to use")

set(SHAPELIB ON CACHE BOOL "GDL: Enable SHAPELIB (for IDLffShape, MAP_CONTINENTS and all geographic data?)")
set(SHAPELIBDIR "" CACHE PATH "GDL: Specify the SHAPELIB directory tree")

set(EXPAT ON CACHE BOOL "GDL: Enable EXPAT (for IDLffXMLSAX)")
set(EXPATDIR "" CACHE PATH "GDL: Specify the libEXPAT-devel directory tree")

if(NOT WIN32)
  set(NCURSESDIR "" CACHE PATH "GDL: Specify the ncurses (or curses) directory tree")
endif(NOT WIN32)

set(OPENMP ON CACHE BOOL "GDL: Enable OpenMP ?")

# Third party libraries
set(JASPERDIR "" CACHE PATH "GDL: Specify the JasPer directory tree")
set(JPEGDIR "" CACHE PATH "GDL: Specify the JPEG directory tree")
set(SZIPDIR "" CACHE PATH "GDL: Specify the SZip directory tree")

set(GDL_DATA_DIR "/share/gnudatalanguage" CACHE PATH "GDL: data directory relative to CMAKE_INSTALL_PREFIX")

# check for 64-bit OS
if(${CMAKE_SIZEOF_VOID_P} EQUAL 8)
    set(HAVE_64BIT_OS 1)
endif(${CMAKE_SIZEOF_VOID_P} EQUAL 8)

# check for endianness, and set macro in case it is big.
TEST_BIG_ENDIAN(IS_BIGENDIAN)

#### check headers and libraries

# dl
check_library_exists(dl dlopen "" HAVE_DL)
if(HAVE_DL)
    set(LIBRARIES ${LIBRARIES} dl)
endif(HAVE_DL)

# malloc stats
check_function_exists(malloc_zone_statistics HAVE_MALLOC_ZONE_STATISTICS)
check_function_exists(sbrk HAVE_SBRK)
check_function_exists(mallinfo HAVE_MALLINFO)

# mallocs
check_include_file(malloc.h HAVE_MALLOC_H)
check_include_file(malloc/malloc.h HAVE_MALLOC_MALLOC_H)

# locale
check_include_file(locale.h HAVE_LOCALE_H)

# std includes..
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(stdlib.h HAVE_STDLIB_H)
check_include_file(string.h HAVE_STRING_H)
check_include_file(strings.h HAVE_STRINGS_H)
check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(unistd.h HAVE_UNISTD_H)
#check_include_file(ext/stdio_filebuf.h HAVE_EXT_STDIO_FILEBUF_H)
#sometimes it compiles but stdio_filebuf is only in gcc-x-y-z directory tree...
# would be silly not to use this functionality if present, so recheck:
if (HAVE_EXT_STDIO_FILEBUF_H)
else (HAVE_EXT_STDIO_FILEBUF_H)
    check_cxx_source_compiles("
#include <ext/stdio_filebuf.h>
int main(int argc, char **argv) {
    int i=0;
    i+=1;
}" REALLY_HAVE_EXT_STDIO_FILEBUF_H)
endif (HAVE_EXT_STDIO_FILEBUF_H)
if (REALLY_HAVE_EXT_STDIO_FILEBUF_H)
message(STATUS "INFO: will use GNU extensions for STDIO (useful for compressed I/O) since it seems accepted by your c++ compiler.")
set (HAVE_EXT_STDIO_FILEBUF_H 1)
endif (REALLY_HAVE_EXT_STDIO_FILEBUF_H)
# dlfcn.h
check_include_file(dlfcn.h HAVE_DLFCN_H)

# inttypes.h
check_include_file(inttypes.h HAVE_INTTYPES_H)

# nexttoward
check_library_exists(m nexttoward "" HAVE_NEXTTOWARD)

# mpi
check_include_file(mpi.h HAVE_MPI_H)

# SA: whithout it compilation of antlr fails if there's a conflicting 
#     version of antlr in system-wide directories
include_directories(src)

if (NOT WIN32)
    # Ncurses MANDATORY for readline on POSIX
    # -DNCURSESDIR=DIR
    set(CMAKE_PREFIX_PATH ${NCURSESDIR})    
    set(CURSES_NEED_NCURSES TRUE)
    find_package(Curses QUIET COMPONENTS initscr)
    mark_as_advanced(CURSES_CURSES_H_PATH CURSES_FORM_LIBRARY CURSES_HAVE_CURSES_H)
    if(CURSES_FOUND)
        set(HAVE_LIBNCURSES 1)
        set(LIBRARIES ${LIBRARIES} ${CURSES_LIBRARIES})
        include_directories(${CURSES_INCLUDE_DIR})
    else(CURSES_FOUND)
        # search for curses
        set(CURSES_NEED_NCURSES FALSE)
        find_package(Curses COMPONENTS initscr)
        set(HAVE_LIBCURSES ${CURSES_FOUND})
        if(CURSES_FOUND)
            set(LIBRARIES ${LIBRARIES} ${CURSES_LIBRARIES})
            include_directories(${CURSES_INCLUDE_DIR})
        else(CURSES_FOUND)
            message(FATAL_ERROR "(N)Curses was not found.\n"
                "Use -DNCURSESDIR=DIR to specify the curses directory tree.\n"
                "(suitable Debian/Ubuntu package: libncurses-dev)\n")
        endif(CURSES_FOUND)
    endif(CURSES_FOUND)
endif (NOT WIN32)

# -DREADLINE=ON|OFF
# -DREADLINEDIR=DIR
if(READLINE)
  set(CMAKE_PREFIX_PATH ${READLINEDIR})
  find_package(Readline QUIET)
  set(HAVE_LIBREADLINE ${READLINE_FOUND})
  if(READLINE_FOUND)
    if(NOT WIN32)
      # readline needs (n)curses (not on Windows)
      set(CMAKE_REQUIRED_LIBRARIES ${CURSES_LIBRARIES})
    endif(NOT WIN32)
    check_library_exists("${READLINE_LIBRARIES}" rl_get_screen_size "" RL_GET_SCREEN_SIZE)
    if(NOT RL_GET_SCREEN_SIZE)
      message(STATUS "WARNING: Older GNU readline without rl_get_screen_size was found.\n"
    "For resized terminals the size might not be updated correctly.\n"
    "If this is a problem please install a recent version of readline.")
    endif(NOT RL_GET_SCREEN_SIZE)
    set(LIBRARIES ${LIBRARIES} ${READLINE_LIBRARIES})
    include_directories(${READLINE_INCLUDE_DIR})
    set(CMAKE_REQUIRED_LIBRARIES)
  else(READLINE_FOUND)
    message(FATAL_ERROR "GNU readline was not found.\n"
      "Use -DREADLINE=no to explicitely disable it.\n"
      "(suitable Debian/Ubuntu package: libreadline-gplv2-dev or libreadline-dev)\n"
      "(suitable Fedora/CentOS package: readline-devel)")
  endif(READLINE_FOUND)
endif(READLINE)

# zlib MANDATORY
# -DZLIBDIR=DIR
set(CMAKE_PREFIX_PATH ${ZLIBDIR})
find_package(ZLIB QUIET)
set(HAVE_LIBZ ${ZLIB_FOUND})
if(ZLIB_FOUND)
    set(LIBRARIES ${LIBRARIES} ${ZLIB_LIBRARIES})
    include_directories(${ZLIB_INCLUDE_DIR})
else(ZLIB_FOUND)
    message(FATAL_ERROR "ZLib library is required but was not found.\n"
    "Use -DZLIBDIR=DIR to specify the zlib directory tree.\n"
        "(suitable Debian/Ubuntu package: zlib1g-dev)\n"
        "(suitable Fedora/CentOS package: zlib-devel)")
endif(ZLIB_FOUND)

# libpng
# -DPNGLIB=ON|OFF
# -DPNGLIBDIR=DIR
if(PNGLIB)
    set(CMAKE_PREFIX_PATH ${PNGLIBDIR})
    find_package(PNG QUIET)
    set(USE_PNGLIB ${PNG_FOUND})
    if(PNG_FOUND)
        set(LIBRARIES ${LIBRARIES} ${PNG_LIBRARIES})
        set(LINK_DIRECTORIES ${LINK_DIRECTORIES} ${PNG_LIBRARY_DIRS})
        include_directories(${PNG_INCLUDE_DIRS})
    else(PNG_FOUND)
        message(FATAL_ERROR "libpng is required but was not found.\n"
        "Use -DPNGLIBDIR=DIR to specify the libpng directory tree.\n"
        "Use -DPNGLIB=OFF to not use it.\n"
                "(suitable Debian/Ubuntu package: libpng-dev)\n"
                "(suitable Fedora/CentOS package: libpng-devel)")
    endif(PNG_FOUND)
endif(PNGLIB)

# openmp
# -DOPENMP=ON|OFF
if(OPENMP)
  find_package(OpenMP QUIET)
  set(USE_OPENMP ${OPENMP_FOUND})
  if(OPENMP_FOUND)
    if(MSVC)
      set(LIBRARIES ${LIBRARIES} vcomp)
    elseif(WIN32)
      set(LIBRARIES ${LIBRARIES} gomp pthread)
    else()
      set(LIBRARIES ${LIBRARIES} ${OpenMP_CXX_FLAGS})
    endif()
  else(OPENMP_FOUND)
    message(FATAL_ERROR "Your compiler does not support OpenMP or OpenMP was not found"
    "note that disabling OpenMP will incur significant performance penalty !\n"
    "Use -DCMAKE_CXX_COMPILER=PATH and -DCMAKE_C_COMPILER=PATH to specify the compiler path."
    "(find_package(OpenMP) will find OpenMP only if both are set !)\n"
    "Use -DOPENMP=OFF to not use it.\n"
    "(suitable Homebrew compiler package: llvm)"
    "(suitable Homebrew library package: libomp)")
  endif(OPENMP_FOUND) 
else(OPENMP)
  # we need to define those semaphore posix symbols, do it with threads libs
  find_package(Threads)
  if(THREADS_FOUND)
    set(LIBRARIES ${LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
  else(THREADS_FOUND)
    check_library_exists(rt sem_unlink "" HAVE_RT)
    if(HAVE_RT)
      set(LIBRARIES ${LIBRARIES} rt)
    else(HAVE_RT)
      message(FATAL_ERROR "Missing a POSIX semaphore symbols (rt or threads).\n")
    endif(HAVE_RT)
  endif(THREADS_FOUND)
endif(OPENMP)

# GSL MANDATORY
# -DGSLDIR=DIR
set(CMAKE_PREFIX_PATH ${GSLDIR})
find_package(GSL QUIET)
set(HAVE_LIBGSL ${GSL_FOUND})
set(HAVE_LIBGSLCBLAS ${GSL_FOUND})
if(GSL_FOUND)
    set(LIBRARIES ${LIBRARIES} ${GSL_LIBRARIES})
    include_directories(${GSL_INCLUDE_DIR})
else(GSL_FOUND)
    message(FATAL_ERROR "Gnu Scientific library (1.7 or higher) and libgslcblas are mandatory.\n"
    "Use -DGSLDIR=DIR to specify the gsl directory tree.\n"
        "(suitable Debian/Ubuntu package: libgsl-dev)\n"
        "(suitable Fedora/CentOS package: gsl-devel)")
endif(GSL_FOUND)

if(UNIX)
    set(CMAKE_PREFIX_PATH ${RPCDIR})
    find_package(RPC QUIET)
    set(HAVE_RPC ${RPC_FOUND})
    if(RPC_FOUND)
        set(LIBRARIES ${LIBRARIES} ${RPC_LIBRARIES})
        include_directories(${RPC_INCLUDE_DIR})
    else(RPC_FOUND)
        message(FATAL_ERROR "RPC support is mandatory.\n"
        "Note that SunRPC has been removed in glibc-2.26 and later, "
        "while being optional in earlier versions. Consider using the "
        "recommended and more modern libtirpc instead.\n"
        "Use -DRPCDIR=DIR to specify the rpc directory tree.\n")
    endif(RPC_FOUND)
elseif (WIN32)
    set(CMAKE_PREFIX_PATH ${XDRDIR})
    find_package(Xdr)
    set(HAVE_LIBXDR ${XDR_FOUND})
    if(XDR_FOUND)
        set(LIBRARIES ${LIBRARIES} ${XDR_LIBRARIES})
        include_directories(${XDR_INCLUDE_DIR})
    else(XDR_FOUND)
        message(FATAL_ERROR "bsd-xdr library is required but was not found.\n"
        "Use -DXDRDIR=DIR to specify the bsd-xdr directory tree.")
    endif(XDR_FOUND)

    set(CMAKE_PREFIX_PATH ${PCREDIR})
    find_package(PCRE)
    set(HAVE_LIBPCRE ${PCRE_FOUND})
    if(PCRE_FOUND)
        set(LIBRARIES ${LIBRARIES} ${PCRE_LIBRARIES})
        include_directories(${PCRE_INCLUDE_DIR})
    else(PCRE_FOUND)
        message(FATAL_ERROR "pcre library is required but was not found.\n"
        "Use -DPCREDIR=DIR to specify the pcre directory tree.")
    endif(PCRE_FOUND)
    LINK_LIBRARIES(shlwapi)
    if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
      add_compile_options(-Wa,-mbig-obj)
      message(STATUS "-Wa,-mbig-obj added to compiler options ")
    endif() 
endif()


# wxWidgets: should be the one graphics library. Presence test is 'wxWidgets_FOUND'
#
#    I suggest to have wxWidgets a 'main' dependency. 
#    thus removing the need of X11 for unix/linux/MacOS, as well as xwin and wingcc/wingdi.
#    Behavior of plots, either in a WIDGET_DRAW or in a WINDOW, will be identical.
#    Provided wxWidgets library exist, the graphic device will be 'X' on all platforms.
#
# -DWXWIDGETS=ON|OFF
# -DWXWIDGETSDIR=DIR
set(wxWidgets_USE_DEBUG OFF) # Use release build
if(WXWIDGETS)
    set(CMAKE_PREFIX_PATH ${WXWIDGETSDIR})
    find_package(wxWidgets COMPONENTS base core adv)
    set(HAVE_LIBWXWIDGETS ${wxWidgets_FOUND})
    if(wxWidgets_FOUND)
        set(LIBRARIES ${LIBRARIES} ${wxWidgets_LIBRARIES})
        set(LINK_DIRECTORIES ${LINK_DIRECTORIES} ${wxWidgets_LIBRARY_DIRS})
        foreach(WXDEF ${wxWidgets_DEFINITIONS})
            add_definitions(-D${WXDEF})
        endforeach(WXDEF ${wxWidgets_DEFINITIONS})
        include_directories(${wxWidgets_INCLUDE_DIRS})
    else(wxWidgets_FOUND)
        message(FATAL_ERROR "wxWidgets are required but were not found.\n"
        "Use -DWXWIDGETSDIR=DIR to specify the wxWidgets directory tree.\n"
        "Use -DWXWIDGETS=OFF to not use it.\n"
                "(suitable Debian/Ubuntu package: libwxgtk3.0-gtk3-dev)\n"
                "(suitable Fedora/CentOS package: wxGTK-devel)")
    endif(wxWidgets_FOUND)
endif(WXWIDGETS)

# X11 : OFF by default since wxWidgets SHOULD take preference in ALL builds.
# Note that X11 is not present under native WIN32. Presence test is 'X11_FOUND'
if(CYGWIN OR NOT WIN32)
    if(X11)
        set(CMAKE_PREFIX_PATH ${X11DIR})
        find_package(X11 QUIET)
        set(HAVE_X ${X11_FOUND})
        if(X11_FOUND)
            set(LIBRARIES ${LIBRARIES} ${X11_LIBRARIES})
            include_directories(${X11_INCLUDE_DIR})
        else(X11_FOUND)
            message(FATAL_ERROR "X11 is required but was not found.\n"
            "Use -DX11DIR=DIR to specify the X11 directory tree.\n"
            "Use -DX11=OFF to not use it.\n"
	    "(suitable Fedora/CentOS package xorg-x11-devel)\n")
        endif(X11_FOUND)
    endif(X11)
    if (X11_FOUND AND NOT wxWidgets_FOUND) 
      message(STATUS "WARNING: GDL is BEST with wxWidgets instead of X11, please consider installing wxWidgets")
    endif (X11_FOUND AND NOT wxWidgets_FOUND) 
endif(CYGWIN OR NOT WIN32)


# PLplot MANDATORY
# -DPLPLOTDIR=DIR
set(CMAKE_PREFIX_PATH ${PLPLOTDIR})
find_package(Plplot QUIET)
set(HAVE_LIBPLPLOTCXXD ${PLPLOT_FOUND})
if(PLPLOT_FOUND)
    set(LIBRARIES ${LIBRARIES} ${PLPLOT_LIBRARIES})
    include_directories(${PLPLOT_INCLUDE_DIR})
else(PLPLOT_FOUND)
    message(FATAL_ERROR "plplot library is required but was not found.\n"
    "Use -DPLPLOTDIR=DIR to specify the plplot directory tree.\n"
        "(suitable Debian/Ubuntu packages: libplplot-dev, [plplot9-driver-xwin])"
        "(suitable Fedora/CentOS package: plplot-devel)")
endif(PLPLOT_FOUND)

if(PLPLOT_FOUND)
    message(STATUS "Warning, if you have plplot version > 5.11 and wxWidgets enabled, please read carefully file README")
    check_library_exists("${PLPLOT_LIBRARIES}" c_plslabelfunc "" HAVE_PLPLOT_SLABELFUNC)
    if(NOT HAVE_PLPLOT_SLABELFUNC)
      message(FATAL_ERROR "please upgrade to plplot version > 5.9.6 (missing plslabelfunc")
    endif()

    set(CMAKE_REQUIRED_INCLUDES "${PLPLOT_INCLUDE_DIR}")
    set(CMAKE_REQUIRED_LIBRARIES "${PLPLOT_LIBRARIES}")

    #the following does not work with clang (???)
        check_library_exists("${PLPLOT_LIBRARIES}" c_plwidth "" HAVE_PLPLOT_WIDTH)
    
    #give it a 2nd try...
    if(NOT HAVE_PLPLOT_WIDTH)
       set(_test_plplot
       "
       #include <plplot/plstream.h>
       int main(int argc, char **argv) {
         plstream *p = new plstream();
         PLFLT w = 0.5;
         p->width(w);
       }
       "
       )
       check_cxx_source_compiles("${_test_plplot}" HAVE_PLPLOT_WIDTH)
       if(HAVE_PLPLOT_WIDTH)
         set(HAVE_PLPLOT_WIDTH 1)
       endif(HAVE_PLPLOT_WIDTH)
    endif(NOT HAVE_PLPLOT_WIDTH)
    
    check_library_exists("${PLPLOT_LIBRARIES}" plstrl "" PLPLOT_PRIVATE_NOT_HIDDEN)
    if(PLPLOT_PRIVATE_NOT_HIDDEN)
      set(PLPLOT_PRIVATE_NOT_HIDDEN 1)
    else(PLPLOT_PRIVATE_NOT_HIDDEN)
      message(STATUS "Using a plplot library without private functions - workarounds will be used.")
    endif(PLPLOT_PRIVATE_NOT_HIDDEN)

    set(_test_plplot2
        "
        #include <plplot/plstream.h>
        int main(){
        PLINT n; 
        PLFLT x,y;
        plcallback::fill(n,&x,&y);
        }
        "
        )    
    check_cxx_source_compiles("${_test_plplot2}" PLPLOT_HAS_PLCALLBACK)
    if(PLPLOT_HAS_PLCALLBACK)
      set(PLPLOT_HAS_PLCALLBACK 1)
    endif(PLPLOT_HAS_PLCALLBACK)
 
    if (INTERACTIVE_GRAPHICS)
      set (_test_wxwidgets
      "
      #include <plplot/plplot.h>
      #include<string.h>
      int main(int argc, char **argv) {
      int numdevs = 128;
      const char **devlongnames = (const char**) malloc(numdevs * sizeof (char*));
      const char **devnames = (const char**) malloc(numdevs * sizeof (char*));
      plgDevs(&devlongnames, &devnames, &numdevs);
      for (int i = 0; i < numdevs; ++i) {if (strcmp(\"wxwidgets\", devnames[i])==0) exit(EXIT_SUCCESS);}
      exit(EXIT_FAILURE);
      }
      "
      )
      set (_test_xwin
      "
      #include <plplot/plplot.h>
      #include<string.h>
      int main(int argc, char **argv) {
      int numdevs = 128;
      const char **devlongnames = (const char**) malloc(numdevs * sizeof (char*));
      const char **devnames = (const char**) malloc(numdevs * sizeof (char*));
      plgDevs(&devlongnames, &devnames, &numdevs);
      for (int i = 0; i < numdevs; ++i) {if (strcmp(\"xwin\", devnames[i])==0) exit(EXIT_SUCCESS);}
      exit(EXIT_FAILURE);
      }
      "
      )
      set (_test_wingcc
      "
      #include <plplot/plplot.h>
      #include<string.h>
      int main(int argc, char **argv) {
      int numdevs = 128;
      const char **devlongnames = (const char**) malloc(numdevs * sizeof (char*));
      const char **devnames = (const char**) malloc(numdevs * sizeof (char*));
      plgDevs(&devlongnames, &devnames, &numdevs);
      for (int i = 0; i < numdevs; ++i) {if ( strcmp(\"wingcc\", devnames[i])==0 || strcmp(\"wingdi\", devnames[i])==0 ) exit(EXIT_SUCCESS);}
      exit(EXIT_FAILURE);
      }
      "
      )
      # if wxWidgets found, we check whether plplot was compiled within wxwidgets
      # if plplot's wxwidgets driver is not in the distribution, and if X11 is not present either (or winggc under WIN32) then
      # it will be a STOP unless INTERACTIVE_GRAPHICS=OFF. Totally noninteractive graphics apps (such as Jupyter GDL kernel)
      # must be compiled with -DINTERACTIVE_GRAPHICS=OFF .
      if (wxWidgets_FOUND)
          check_c_source_runs("${_test_wxwidgets}" HAVE_PLPLOT_WXWIDGETS_DRIVER)
      endif (wxWidgets_FOUND)
      #idem for X11
      if(CYGWIN OR NOT WIN32)
         if (X11_FOUND)
            check_c_source_runs("${_test_xwin}" HAVE_PLPLOT_X11_DRIVER)
         endif (X11_FOUND)
      else (CYGWIN OR NOT WIN32)  #WIN32: check wingcc
          check_c_source_runs("${_test_wingcc}" HAVE_PLPLOT_X11_DRIVER) #same env var different meaning
      endif(CYGWIN OR NOT WIN32)

      if (NOT HAVE_PLPLOT_WXWIDGETS_DRIVER AND NOT HAVE_PLPLOT_X11_DRIVER ) 
           if (wxWidgets_FOUND) 
             message(FATAL_ERROR "PlPlot was compiled without needed wxwidgets driver, aborting. Do you mean -DINTERACTIVE_GRAPHICS=OFF ?")
           else (wxWidgets_FOUND)
             message(FATAL_ERROR "PlPlot was compiled without xwin (or wingcc on Windows) driver, aborting. Do you mean -DINTERACTIVE_GRAPHICS=OFF ?")
           endif (wxWidgets_FOUND)
      endif (NOT HAVE_PLPLOT_WXWIDGETS_DRIVER AND NOT HAVE_PLPLOT_X11_DRIVER)
    endif (INTERACTIVE_GRAPHICS)
endif(PLPLOT_FOUND)
    
# GraphicsMagick (GM) is an alternative to the classical ImageMagick Lib (IM).
# It was experienced that GM was more stable in time than IM
#
# -DGRAPHICSMAGICK=ON|OFF
# -DGRAPHICSMAGICKDIR=DIR
#
if(GRAPHICSMAGICK)
    set(CMAKE_PREFIX_PATH ${GRAPHICSMAGICKDIR})
    find_package(GraphicsMagick QUIET)
    set(USE_MAGICK ${GRAPHICSMAGICK_FOUND})
    if(GRAPHICSMAGICK_FOUND)
        include_directories(${GRAPHICSMAGICK_INCLUDE_DIR})  
        set(LIBRARIES ${LIBRARIES} ${GRAPHICSMAGICK_LIBRARIES})     
    else(GRAPHICSMAGICK_FOUND)
        message( STATUS
        "GRAPHICSMAGICK is strongly suggested but was not found.
        Use -DGRAPHICSMAGICKDIR=DIR to specify the GraphicsMagick directory tree.
        Use -DGRAPHICSMAGICK=OFF to not use it.
                (suitable Fedora/CentOS package: GraphicsMagick-c++-devel
           Debian/Ubuntu package: libgraphicsmagick++1-dev)\n")
        message( STATUS 
        "Looking for ImageMagick")
    endif(GRAPHICSMAGICK_FOUND)
endif(GRAPHICSMAGICK)
#
# if GM found, we do not look for IM
#
if(GRAPHICSMAGICK_FOUND)
    if (MAGICK)
       message(STATUS "INFO: We prefer to use GraphicsMagick than ImageMagick")
       set(MAGICK OFF)
        endif(MAGICK)
endif(GRAPHICSMAGICK_FOUND)

# ImageMagick
# -DMAGICK=ON|OFF
# -DMAGICKDIR=DIR
if(MAGICK)
  set(CMAKE_PREFIX_PATH ${MAGICKDIR})
  find_package(ImageMagick QUIET COMPONENTS Magick++ MagickWand MagickCore)
  mark_as_advanced(ImageMagick_EXECUTABLE_DIR ImageMagick_Magick++_INCLUDE_DIR ImageMagick_Magick++_LIBRARY
    ImageMagick_MagickCore_INCLUDE_DIR ImageMagick_MagickCore_LIBRARY ImageMagick_MagickWand_INCLUDE_DIR ImageMagick_MagickWand_LIBRARY)
  set(USE_MAGICK ${ImageMagick_FOUND})
  if(ImageMagick_FOUND)
    find_program(MAGICKXXCONFIG Magick++-config)
    if(MAGICKXXCONFIG)
      execute_process(COMMAND ${MAGICKXXCONFIG} "--libs" OUTPUT_VARIABLE MAGICKXXCONFIGLIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
      set(LIBRARIES ${LIBRARIES} ${MAGICKXXCONFIGLIBS})     
    else(MAGICKXXCONFIG)
      message(FATAL_ERROR "ImageMagick is required but was not found (Magick++-config).\n"
    "Use -DMAGICKDIR=DIR to specify the ImageMagick directory.\n"
    "Use -DMAGICK=OFF to not use it.\n"
        "(suitable Debian/Ubuntu package: libmagick++-dev)\n"
        "(suitable Fedora/CentOS package: ImageMagick-c++-devel)")
    endif(MAGICKXXCONFIG)
    set(LIBRARIES ${LIBRARIES} ${ImageMagick_LIBRARIES})
    include_directories(${ImageMagick_INCLUDE_DIRS})
    set(MAGICK_LIBRARIES ${ImageMagick_LIBRARIES})
  else(ImageMagick_FOUND)
    message(FATAL_ERROR "ImageMagick is required but was not found.\n"
      "Use -DMAGICKDIR=DIR to specify the ImageMagick directory.\n"
      "Use -DMAGICK=OFF to not use it.\n"
      "(suitable Debian/Ubuntu package: libmagick++-dev)\n"
      "(suitable Fedora/CentOS package: ImageMagick-c++-devel)")
  endif(ImageMagick_FOUND)
endif(MAGICK)

## not valid anymore I bet ## if GM or IM activated, we check whether Plplot is OK for that
## not valid anymore I bet #if(USE_MAGICK AND NOT WIN32)
## not valid anymore I bet #  check_library_exists("${PLPLOT_LIBRARIES}" plGetDrvDir "" HAVE_PLGETDRVDIR)
## not valid anymore I bet #  if(HAVE_PLGETDRVDIR)
## not valid anymore I bet #    message(STATUS "WARNING: Dynamic drivers may have to be disabled in plplot.\n"
## not valid anymore I bet #      "To make plplot work with ImageMagick:\n"
## not valid anymore I bet #      "use -DENABLE_DYNDRIVERS=OFF cmake option when compiling plplot or\n"
## not valid anymore I bet #      "use -DMAGICK=OFF here to not use ImageMagick in GDL.")
## not valid anymore I bet #  endif(HAVE_PLGETDRVDIR)
## not valid anymore I bet #endif(USE_MAGICK AND NOT WIN32)


# libtiff
# -DTIFF=ON|OFF
# -DTIFFDIR=DIR
if(TIFF)
    set(CMAKE_PREFIX_PATH ${TIFFDIR})
    find_package(TIFF 4.0.3 QUIET)
    set(USE_TIFF ${TIFF_FOUND})
    if(TIFF_FOUND)
        include_directories(${TIFF_INCLUDE_DIR})
        set(LIBRARIES ${LIBRARIES} ${TIFF_LIBRARIES})
    else(TIFF_FOUND)
        message(FATAL_ERROR
        "libtiff v4.0.3 or newer is required for TIFF support but was not found.\n"
        "Use -DTIFFDIR=DIR to specify the libtiff directory tree.\n"
        "Use -DTIFF=OFF to not use it.")
    endif(TIFF_FOUND)
endif(TIFF)

# libgeotiff
# -DGEOTIFF=ON|OFF
# -DGEOTIFFDIR=DIR
if(GEOTIFF AND USE_TIFF)
    set(CMAKE_PREFIX_PATH ${GEOTIFFDIR})
    find_package(GeoTIFF QUIET)
    set(USE_GEOTIFF ${GEOTIFF_FOUND})
    if(GEOTIFF_FOUND)
        include_directories(${GEOTIFF_INCLUDE_DIR})
        set(LIBRARIES ${LIBRARIES} ${GEOTIFF_LIBRARIES})
    else(GEOTIFF_FOUND)
        message(FATAL_ERROR
        "libgeotiff is required for GeoTIFF support but was not found.\n"
        "Use -DGEOTIFFDIR=DIR to specify the libgeotiff directory tree.\n"
        "Use -DGEOTIFF=OFF to not use it.\n"
	"(suitable Debian/Ubuntu package: libgeotiff-dev)\n"
	"(suitable Fedora/CentOS package: libgeotiff-devel)")
    endif(GEOTIFF_FOUND)
endif(GEOTIFF AND USE_TIFF)

# netCDF
# -DNETCDF=ON|OFF
# -DNETCDFDIR=DIR
if(NETCDF)
    set(CMAKE_PREFIX_PATH ${NETCDFDIR})
    find_package(NetCDF QUIET)
    if(NETCDF_FOUND)
        set(CMAKE_REQUIRED_INCLUDES ${NETCDF_INCLUDE_DIRS})
        check_include_file_cxx(netcdf.h HAVE_NETCDF_H)
        if(HAVE_NETCDF_H)
            set(LIBRARIES ${LIBRARIES} ${NETCDF_LIBRARIES})
            set(LINK_DIRECTORIES ${LINK_DIRECTORIES} ${NETCDF_LIBRARY_DIRS})
            include_directories(${NETCDF_INCLUDE_DIRS})
            set(USE_NETCDF 1)
        else(HAVE_NETCDF_H)
            message(FATAL_ERROR "NetCDF installation seems not to be usable.\n"
            "This suggests a conflicting netCDF-HDF4 installation e.g.\n"
            "- Uninstalling HDF4 after installation of NetCDF.\n"
            "- Installing NetCDF before HDF4.")
        endif(HAVE_NETCDF_H)
        set(CMAKE_REQUIRED_INCLUDES)
        #
        # are extensions NetCDF-4 available ??
        check_library_exists("${NETCDF_LIBRARIES}" nc_inq_grps "" HAVE_NETCDF4)
        if(HAVE_NETCDF4)
          set(USE_NETCDF4 1)
        else(HAVE_NETCDF4)
          message(STATUS "warning, you don't have NetCDF-4 version"
            "some new NetCDF capabilities in NetCDF-4 (related to Groups) will not be usable")
        endif(HAVE_NETCDF4)
    else(NETCDF_FOUND)
        message(FATAL_ERROR "NetCDF version 3.5.1 or later is required but was not found.\n"
        "Use -DNETCDFDIR=DIR to specify the netcdf directory tree.\n"
        "Use -DNETCDF=OFF to not use it.\n"
                "(suitable Debian/Ubuntu package: libnetcdf-dev)\n"
                "(suitable Fedora/CentOS package: netcdf-devel)")
    endif(NETCDF_FOUND)
endif(NETCDF)

# hdf4
# -DHDF=ON|OFF
# -DHDFDIR=DIR
if(HDF)
    set(CMAKE_PREFIX_PATH ${HDFDIR} ${JPEGDIR} ${SZIPDIR})
    find_package(HDF QUIET)
    set(USE_HDF ${HDF_FOUND})
    if(HDF_FOUND)
        if(NETCDF)
            set(CMAKE_REQUIRED_LIBRARIES ${HDF_EXTRA_LIBRARIES})
                        check_library_exists("${HDF_LIBRARIES};${RPC_LIBRARIES}" sd_nccreate "" SD_NCCREATE)
            if(NOT SD_NCCREATE)
                message(FATAL_ERROR "HDF4 needs to be configured with the --disable-netcdf option "
                "in order to be used with the original netCDF library.")
            endif(NOT SD_NCCREATE)
            set(CMAKE_REQUIRED_LIBRARIES)
        endif(NETCDF)
        set(HDF_LIBRARIES ${HDF_LIBRARIES} ${HDF_EXTRA_LIBRARIES})
        set(LIBRARIES ${LIBRARIES} ${HDF_LIBRARIES})
        include_directories(${HDF_INCLUDE_DIR})
    else(HDF_FOUND)
        message(FATAL_ERROR "HDF4 libraries were not found.\n"
        "Use -DHDFDIR=DIR to specify the HDF directory tree.\n"
        "Use -DHDF=OFF to not use it.\n"
                "(suitable Debian/Ubuntu package: libhdf4-alt-dev)\n"
                "(suitable Fedora/CentOS package: hdf-devel)\n"
        "You can use -DJPEGDIR=DIR to specify the JPEG directory tree. "
        "You can also use -DSZIPDIR=DIR to specify SZip directory tree if "
        "HDF was compiled with SZip support.")
    endif(HDF_FOUND)
endif(HDF)

# hdf5
# -DHDF5=ON|OFF
# -DHDF5DIR=DIR
if(HDF5)
    set(CMAKE_PREFIX_PATH ${HDF5DIR} ${SZIPDIR})
    find_package(HDF5 QUIET)
    set(USE_HDF5 ${HDF5_FOUND})
    if(HDF5_FOUND)
        set(LIBRARIES ${LIBRARIES} ${HDF5_LIBRARIES})
        include_directories(${HDF5_INCLUDE_DIRS})
        find_package(MPI QUIET)
        if(MPI_FOUND)
            include_directories(${MPI_INCLUDE_PATH})
            set(LIBRARIES ${LIBRARIES} ${MPI_LIBRARIES})    
        endif(MPI_FOUND)
    else(HDF5_FOUND)
        message(FATAL_ERROR "HDF version 5 is required but was not found.\n"
        "Use -DHDF5DIR=DIR to specify the HDF5 directory tree.\n"
        "Use -DHDF5=OFF to not use it.\n"
                "(suitable Debian/Ubuntu package: libhdf5-serial-dev)\n"
                "(suitable Fedora/CentOS package: hdf5-devel)")
    endif(HDF5_FOUND)
endif(HDF5)

# fftw
# -DFFTW=ON|OFF
# -DFFTWDIR=DIR
if(FFTW)
    set(CMAKE_PREFIX_PATH ${FFTWDIR})
    find_package(FFTW QUIET)
    set(USE_FFTW ${FFTW_FOUND})
    if(FFTW_FOUND)
        set(LIBRARIES ${LIBRARIES} ${FFTW_LIBRARIES})
        include_directories(${FFTW_INCLUDE_DIR})
    else(FFTW_FOUND)
        message(FATAL_ERROR "FFTW3 is required but was not found.\n"
        "Use -DFFTWDIR=DIR to specify the FFTW directory tree.\n"
        "Use -DFFTW=OFF to not use it.\n"
                "(suitable Debian/Ubuntu package: libfftw3-dev)\n"
                "(suitable Fedora/CentOS package: fftw-devel)")
    endif(FFTW_FOUND)
endif(FFTW)

# PROJ
# -DLIBPROJ=ON|OFF
# -DLIBPROJDIR=DIR
if(LIBPROJ)
    set(CMAKE_PREFIX_PATH ${LIBPROJDIR})
    find_package(LIBPROJ)
    set(USE_LIBPROJ ${LIBPROJ_FOUND})

    if(LIBPROJ_FOUND)  
        set(LIBRARIES ${LIBRARIES} ${LIBPROJ_LIBRARIES})
        include_directories(${LIBPROJ_INCLUDE_DIR})
        set(LIBPROJ_MAJOR_VERSION ${LIBPROJ_MAJOR_VERSION})
    else(LIBPROJ_FOUND)
        message(FATAL_ERROR "PROJ is required but was not found.\n"
        "Use -DLIBPROJDIR=DIR to specify the PROJ directory tree.\n"
        "Use -DLIBPROJ=OFF to not use it.\n"
        "(suitable Debian/Ubuntu package: libproj-dev)\n"
        "(suitable Fedora/CentOS package: proj-devel)")
    endif(LIBPROJ_FOUND)   
endif(LIBPROJ)

# MPI (experimental)
# -DMPI=ON|OFF
# -DMPIDIR=DIR
if(MPI)
	set(CMAKE_PREFIX_PATH ${MPIDIR})
	find_package(MPI QUIET)
	set(USE_MPI ${MPI_FOUND})
	if(MPI_FOUND)
		include_directories(${MPI_INCLUDE_PATH})	
		set(LIBRARIES ${LIBRARIES} ${MPI_LIBRARIES})
	else(MPI_FOUND)
		message(FATAL_ERROR "MPI is required but was not found.\n"
		"Use -DMPIDIR=DIR to specify the MPI directory tree.\n"
		"Use -DMPI=OFF to not use it.")
	endif(MPI_FOUND)	
endif(MPI)
# python
# -DPYTHON=ON|OFF
# -DPYTHON_MODULE
# -DPYTHONDIR=DIR
# -DPYTHONVERSION=VERSION
if(PYTHON OR PYTHON_MODULE)
    if(PYTHON)
        set(PYTHONMSG "Use -DPYTHON=OFF to disable Python support.\n")
    endif(PYTHON)
    if(PYTHON_MODULE)
        set(PYTHON_MODULEMSG "Use -DPYTHON_MODULE=OFF to disable Python module.\n")
    endif(PYTHON_MODULE)
    if (${CMAKE_VERSION} VERSION_LESS "3.14")
        if (DEFINED ENV{PYTHON_EXECUTABLE})
            set(PYTHON_EXECUTABLE $ENV{PYTHON_EXECUTABLE})
        endif()
        find_package(PythonInterp ${PYTHONVERSION})
        find_package(PythonLibs ${PYTHONVERSION})
        execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import numpy as np; print(np.get_include())" RESULT_VARIABLE ret OUTPUT_VARIABLE Python_NumPy_INCLUDE_DIRS ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
        if (PYTHONINTERP_FOUND AND PYTHONLIBS_FOUND)
            set(Python_LIBRARIES ${PYTHON_LIBRARIES})
            set(Python_INCLUDE_DIRS ${PYTHON_INCLUDE_DIRS})
            set(Python_Interpreter_FOUND ${PYTHONINTERP_FOUND})
            set(Python_Development_FOUND ${PYTHONLIBS_FOUND})
        endif()
        if (ret EQUAL 0)
            set(Python_NumPy_FOUND TRUE)
        endif()
    else()
        if (DEFINED ENV{PYTHON_EXECUTABLE})
            set(Python_EXECUTABLE $ENV{PYTHON_EXECUTABLE})
        endif()
        find_package(Python ${PYTHONVERSION} COMPONENTS Interpreter Development NumPy)
    endif()
    if(Python_Interpreter_FOUND AND Python_Development_FOUND)
        if(NOT Python_NumPy_FOUND)
            message(FATAL_ERROR "Python NumPy package was not found.\n"
            "${PYTHONMSG} ${PYTHON_MODULEMSG}")
        endif()
        set(USE_PYTHON ${Python_NumPy_FOUND})
        set(LIBRARIES ${LIBRARIES} ${Python_LIBRARIES})
        include_directories(${Python_INCLUDE_DIRS} ${Python_NumPy_INCLUDE_DIRS}) 
    else()
        message(FATAL_ERROR "Python is required but was not found.\n"
        "Use -DPYTHONDIR=DIR to specify the Python directory tree.\n"
                "Use -DPYTHONVERSION=VERSION to help searching for the right version.\n"
                "(suitable Debian/Ubuntu package: python-dev)\n"
                "(suitable Fedora/CentOS package: python-devel)\n"
        "${PYTHONMSG} ${PYTHON_MODULEMSG}")
    endif()
endif(PYTHON OR PYTHON_MODULE)

# udunits-2
# -DUDUNITS2=ON|OFF
# -DUDUNITS2DIR=DIR
if(UDUNITS2)
    set(CMAKE_PREFIX_PATH ${UDUNITS2DIR})
    find_package(Udunits2 QUIET)
    set(USE_UDUNITS ${UDUNITS2_FOUND})
    if(UDUNITS2_FOUND)
        set(LIBRARIES ${LIBRARIES} ${UDUNITS2_LIBRARIES})
        include_directories(${UDUNITS2_INCLUDE_DIR}) 
    else(UDUNITS2_FOUND)
        message(FATAL_ERROR "UDUNITS-2 is required but was not found.\n"
        "Use -DUDUNITS2DIR=DIR to specify the Udunits2 directory tree.\n"
        "Use -DUDUNITS2=OFF to not use it.\n"
                "(suitable Debian/Ubuntu package: libudunits2-dev)\n"
                "(suitable Fedora/CentOS package: udunits2-devel)")
    endif(UDUNITS2_FOUND)
endif(UDUNITS2)

# eigen3
# -DEIGEN3=ON|OFF
# -DEIGEN3DIR=DIR
if(EIGEN3)
  # cleaning Cache ...
  if (EIGEN3DIR)
    unset(EIGEN3_INCLUDE_DIR CACHE)
  endif(EIGEN3DIR)
  #
  set(CMAKE_PREFIX_PATH ${EIGEN3DIR})
  find_package(Eigen3 3.2.4 QUIET)
  if(EIGEN3_TOO_OLD)
    # on iCore 3/5/7 we must have Eigen >=3.2.4 to avoid fatal error in "test_matrix_multiply.pro"
    message(FATAL_ERROR "\nEIGEN3 is required but the version found is TOO OLD."
      " Please download a recent version (>=3.2.4) in a local directory."
      " Then use -DEIGEN3DIR=DIR to specify the Eigen3 local directory tree.\n"
      "Use -DEIGEN3=OFF to not use it.\n")
  else(EIGEN3_TOO_OLD)   
    set(USE_EIGEN ${EIGEN3_FOUND})
    if(EIGEN3_FOUND)
      include_directories(${EIGEN3_INCLUDE_DIR})
      if(MINGW)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-ipa-cp-clone")
      endif(MINGW)
    else(EIGEN3_FOUND)
      message(FATAL_ERROR "EIGEN3 is required but was not found.\n"
        "Use -DEIGEN3DIR=DIR to specify the Eigen3 directory tree.\n"
        "Use -DEIGEN3=OFF to not use it.\n"
            "(suitable Fedora/CentOS package: eigen3-devel)\n"
            "(suitable Debian/Ubuntu package: libeigen3-dev)")
    endif(EIGEN3_FOUND)
      endif(EIGEN3_TOO_OLD)
endif(EIGEN3)

# grib
# -DGRIB=ON|OFF
# -DGRIBDIR=DIR
if(GRIB)
    set(CMAKE_PREFIX_PATH ${GRIBDIR} ${JASPERDIR} ${JPEGDIR})
    find_package(Grib QUIET)
    set(USE_GRIB ${GRIB_FOUND})
    if(GRIB_FOUND)
        set(LIBRARIES ${LIBRARIES} ${GRIB_LIBRARIES})
        include_directories(${GRIB_INCLUDE_DIR})
    else(GRIB_FOUND)
        message(FATAL_ERROR "ECMWF ecCodes is required but was not found.\n"
        "Use -DGRIBDIR=DIR to specify the ecCodes or GRIB API directory tree.\n"
        "Use -DGRIB=OFF to not use it.\n"
        "Use -DJASPERDIR=DIR and|or -DJPEGDIR=DIR to specify "
        "the directory trees of JasPer and openJPEG libraries.\n"
                "(suitable Debian/Ubuntu package: libeccodes-dev)\n"
                "(suitable Fedora/CentOS package: eccodes-devel)\n"
        "(suitable Homebrew package: eccodes)\n")
    endif(GRIB_FOUND)
endif(GRIB)

# Qhull (enable TRIANGULATE and QHULL commands)
# -DQHULL=ON|OFF
# -DQHULLDIR=DIR
if(QHULL)
  set(CMAKE_PREFIX_PATH ${QHULLDIR})
  find_package(QHULL QUIET)
  set(HAVE_QHULL ${QHULL_FOUND})
  if(QHULL_FOUND)
      set(LIBRARIES ${LIBRARIES} ${QHULL_LIBRARIES})
      include_directories(${QHULL_INCLUDE_DIR})
  else(QHULL_FOUND)
      message(FATAL_ERROR "QHULL is required but was not found.\n"
      "Use -DQHULLDIR=DIR to specify the QHULL directory tree.\n"
      "Use -DQHULL=OFF to not use it.\n"
                "(suitable Fedora/CentOS package libqhull-devel)")
  endif(QHULL_FOUND)
endif(QHULL)


# GLPK for Simplex ( Linear programming )
# GLPK (enable SIMPLEX command)
# -DGLPK=ON|OFF
# -DGLPKDIR=DIR
if(GLPK)
  set(CMAKE_PREFIX_PATH ${GLPKDIR})
  find_package(GLPK QUIET)
  set(USE_GLPK ${GLPK_FOUND})
  if(GLPK_FOUND)
    set(LIBRARIES ${LIBRARIES} ${GLPK_LIBRARIES})
    include_directories(${GLPK_INCLUDE_DIR})
  else(GLPK_FOUND)
    message(FATAL_ERROR "GLPK (Gnu Linear Programming Kit) is required but was not found.\n"
      "Use -DGLPKDIR=DIR to specify the GLPK-devel directory tree.\n"
      "Use -DGLPK=OFF to not use it.\n"
      "(suitable Debian/Ubuntu package: libglpk-dev)\n"
      "(suitable Fedora/CentOS package: glpk-devel)\n")
  endif(GLPK_FOUND)
endif(GLPK)
#check_include_file(glpk.h HAVE_GLPK)

# SHAPELIB for all geo/gra/phy/sical data (shapelib format, IDLffShape
# -DSHAPELIB=ON|OFF
# -DSHAPELIBDIR=DIR
if(SHAPELIB)
  set(CMAKE_PREFIX_PATH ${SHAPELIBDIR})
  find_package(SHAPELIB QUIET)
  set(USE_SHAPELIB ${SHAPELIB_FOUND})
  if(SHAPELIB_FOUND)
    set(LIBRARIES ${LIBRARIES} ${SHAPELIB_LIBRARIES})
    include_directories(${SHAPELIB_INCLUDE_DIR})
  else(SHAPELIB_FOUND)
    message(FATAL_ERROR "SHAPELIB (http://shapelib.maptools.org/)  is required but was not found.\n"
      "Use -DSHAPELIBDIR=DIR to specify the SHAPELIB-devel directory tree.\n"
      "Use -DSHAPELIB=OFF to not use it.\n"
      "shapelib is often in package libshp-devel.\n"
      "(suitable Debian/Ubuntu package: libshp-dev)\n"
      "(suitable Fedora/CentOS package: shapelib-devel)\n")
  endif(SHAPELIB_FOUND)
endif(SHAPELIB)
# EXPAT for IDLffXMLSAX and IDLffXMLDOM
# -DEXPAT=ON|OFF
# -DEXPATDIR=DIR
if(EXPAT)
  set(CMAKE_PREFIX_PATH ${EXPATDIR})
  find_package(EXPAT QUIET)
  set(USE_EXPAT ${EXPAT_FOUND})
  if(EXPAT_FOUND)
    set(LIBRARIES ${LIBRARIES} ${EXPAT_LIBRARIES})
    include_directories(${EXPAT_INCLUDE_DIR})
  else(EXPAT_FOUND)
    message(FATAL_ERROR "EXPAT is required but was not found.\n"
      "Use -DEXPATDIR=DIR to specify the LibXML2-devel directory tree.\n"
      "Use -DEXPAT=OFF to not use it.\n"
      "EXPAT is often in package libEXPAT-devel."
      "(suitable Debian/Ubuntu package: libexpat1-dev)\n"
      "(suitable Fedora/CentOS package: expat-devel)\n")
  endif(EXPAT_FOUND)
endif(EXPAT)

add_subdirectory(src)

if(NOT PYTHON_MODULE)
    add_subdirectory(testsuite)
endif(NOT PYTHON_MODULE)

install(FILES ${CMAKE_SOURCE_DIR}/AUTHORS ${CMAKE_SOURCE_DIR}/README DESTINATION ${CMAKE_INSTALL_PREFIX}/${GDL_DATA_DIR})
install(FILES ${CMAKE_SOURCE_DIR}/doc/gdl.1 DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1)

# substitute variables in configure.h.cmake and move it to configure.h
configure_file(${CMAKE_SOURCE_DIR}/config.h.cmake ${CMAKE_BINARY_DIR}/config.h)

# create a link src/gdl -> ${BUILD}/.../gdl
# for windows, just copy the file
if(NOT PYTHON_MODULE)
    set(GDLLOCATION $<TARGET_FILE:gdl>)
    if(WIN32)
    add_custom_target(copy_gdl ALL ${CMAKE_COMMAND} -E copy ${GDLLOCATION} ${CMAKE_SOURCE_DIR}/src/gdl DEPENDS gdl)
    else(WIN32)
    add_custom_target(symlink_gdl ALL ${CMAKE_COMMAND} -E create_symlink ${GDLLOCATION} ${CMAKE_SOURCE_DIR}/src/gdl DEPENDS gdl)
    endif(WIN32)
endif(NOT PYTHON_MODULE)

# display macro
macro(MODULE MOD LIBNAME NAME)
    if(${MOD})
      if(${LIBNAME}_INCLUDE_DIR)
        message("${NAME}         ON (libs:${${LIBNAME}_LIBRARIES}; headers:${${LIBNAME}_INCLUDE_DIR})")
      else()
        message("${NAME}         ON (libs:${${LIBNAME}_LIBRARIES}; headers:${${LIBNAME}_INCLUDE_DIRS})")
      endif()
    else(${MOD})
        message("${NAME}         OFF")
    endif(${MOD})
endmacro(MODULE)

# python
if(PYTHON_MODULE)
    set(BUILDTYPE "Python Module")
else(PYTHON_MODULE)
    set(BUILDTYPE "Standalone")
endif(PYTHON_MODULE)


#
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release CACHE STRING
    "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
    FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

if(MSVC)
    SET(MACHINE_ARCH ${MSVC_C_ARCHITECTURE_ID})
    IF(NOT MACHINE_ARCH)
  SET(MACHINE_ARCH ${MSVC_CXX_ARCHITECTURE_ID})
    ENDIF(NOT MACHINE_ARCH)
    set_target_properties(gdl PROPERTIES LINK_FLAGS "/machine:${MACHINE_ARCH}")
endif(MSVC)

# AC, 12-oct-2011, solved by Marc 
# set_target_properties(gdl PROPERTIES LINK_FLAGS "-Wl,-z,muldefs")
# set_target_properties(gdl PROPERTIES LINK_FLAGS "-z muldefs")
#
if(CMAKE_BUILD_TYPE STREQUAL None OR NOT CMAKE_BUILD_TYPE)
    set(FLAGS ${CMAKE_CXX_FLAGS})
elseif(CMAKE_BUILD_TYPE STREQUAL Debug)
    set(FLAGS ${CMAKE_CXX_FLAGS_DEBUG})
elseif(CMAKE_BUILD_TYPE STREQUAL Release)
    set(FLAGS ${CMAKE_CXX_FLAGS_RELEASE})
elseif(CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
    set(FLAGS ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
elseif(CMAKE_BUILD_TYPE STREQUAL MinSizeRel)
    set(FLAGS ${CMAKE_CXX_FLAGS_MINSIZEREL})
endif(CMAKE_BUILD_TYPE STREQUAL None OR NOT CMAKE_BUILD_TYPE)

message(STATUS
"Summary

GDL - GNU DATA LANGUAGE [${BUILDTYPE}]
System                 ${CMAKE_HOST_SYSTEM}
Files generated        ${CMAKE_GENERATOR}
Installation prefix    ${CMAKE_INSTALL_PREFIX}
C++ compiler           ${CMAKE_CXX_COMPILER} ${FLAGS}")
message("")
message(STATUS "Options")
message("")
message("Interactive plots: ${INTERACTIVE_GRAPHICS}")
message("Widgets support: ${wxWidgets_FOUND}")
message("")

if(OPENMP AND OPENMP_FOUND)
    message("OpenMP support         ON (flag: ${OpenMP_CXX_FLAGS})")
else(OPENMP AND OPENMP_FOUND)
    message("OpenMP support         OFF")
endif(OPENMP AND OPENMP_FOUND)
module(WXWIDGETS wxWidgets "WxWidgets     ")
if (GRAPHICSMAGICK_FOUND)
  module(GRAPHICSMAGICK GRAPHICSMAGICK "GRAPHICSMAGICK")
else (GRAPHICSMAGICK_FOUND)
  module(MAGICK    MAGICK    "ImageMagick   ")
endif (GRAPHICSMAGICK_FOUND)
module(TIFF      TIFF      "TIFF          ")
module(GEOTIFF   GEOTIFF   "GeoTIFF       ")
module(NETCDF    NETCDF    "NetCDF        ")
module(HDF       HDF       "HDF4          ")
module(HDF5      HDF5      "HDF5          ")
module(FFTW      FFTW      "FFTW          ")
module(MPI       MPI       "MPI           ")
module(LIBPROJ   LIBPROJ   "PROJ          ")
module(PYTHON    Python    "Python        ")
module(UDUNITS2  UDUNITS2  "UDUNITS-2     ")
module(EIGEN3    EIGEN3    "EIGEN3        ")
module(GRIB      GRIB      "GRIB          ")
module(QHULL     QHULL     "QHULL         ")
module(GLPK      GLPK      "GLPK          ")
module(SHAPELIB  SHAPELIB  "SHAPELIB      ")
module(EXPAT     EXPAT     "EXPAT         ")
if (INTERACTIVE_GRAPHICS) 
if( CYGWIN OR NOT WIN32  )
  module(X11     X11     "Xlib          ")
endif( CYGWIN OR NOT WIN32)
endif (INTERACTIVE_GRAPHICS)
module(PNGLIB    PNG       "libpng        ")
message("")
message(STATUS "Mandatory modules")
set(PLPLOT ON)
set(READLINE ON)
set(GSL ON)
set(ZLIB ON)
if(NOT WIN32)
  set(CURSES ON)
  set(RPC ON)
  endif(NOT WIN32)
module(PLPLOT    PLPLOT    "Plplot        ")
module(READLINE  READLINE  "GNU Readline  ")
module(GSL       GSL       "GSL           ")
module(ZLIB      ZLIB      "Zlib          ")
if(NOT WIN32)
  module(CURSES    CURSES    "(N)curses     ")
  module(RPC       RPC       "RPC           ")
endif(NOT WIN32)
message("")
message("GDLDEV mode: ${GDLDEV} (use -DGDLDEV=ON to enable work-in-progress tests)")
message("")

# do we have a Conda env. around ? see issues #780
if ((DEFINED ENV{CONDA_PREFIX}) OR (DEFINED ENV{CONDA_EXE}))
  message("==========================================================================\n"
          "== WARNING !!        {Ana|mini|}conda activated env. detected !         ==\n"
          "== To avoid possible conflicts between local libs. and the sytem libs,  ==\n"
          "== we advice you to remove Conda related paths from you $PATH           ==\n"
          "== or run this CMake file in a clean terminal without Conda Env.        ==\n"
          "==========================================================================\n")
endif()
