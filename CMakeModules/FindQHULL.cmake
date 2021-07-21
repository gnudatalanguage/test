# Find QHULL
#
# This sets the following variables:
# QHULL_FOUND - True if QHULL was found.
# QHULL_INCLUDE_DIRS - Directories containing the QHULL include files.
# QHULL_LIBRARIES - Libraries needed to use QHULL.

find_path(QHULL_INCLUDE_DIR NAMES libqhullcpp/Qhull.h
        HINTS ${CMAKE_PREFIX_PATH}/src /usr/local/include /usr/include
        PATH_SUFFIXES qhull src/libqhull libqhull include
        )

if(QHULL_INCLUDE_DIR)
    find_library(QHULL_R_LIBRARY NAMES qhull_r HINTS ${CMAKE_PREFIX_PATH})
    find_library(QHULL_CPP_LIBRARY NAMES qhullcpp HINTS ${CMAKE_PREFIX_PATH})

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(
        QHULL
        REQUIRED_VARS QHULL_R_LIBRARY QHULL_CPP_LIBRARY QHULL_INCLUDE_DIR
        )

    if(QHULL_R_LIBRARY AND QHULL_CPP_LIBRARY)
        SET(QHULL_FOUND TRUE)
        SET(QHULL_INCLUDE_DIRS ${QHULL_INCLUDE_DIR})
        SET(QHULL_LIBRARIES ${QHULL_CPP_LIBRARY} ${QHULL_R_LIBRARY})
    endif()
else(QHULL_INCLUDE_DIR)
    message(STATUS "FindQHull: Could not find Qhull.h")
endif(QHULL_INCLUDE_DIR)
