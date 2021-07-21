set(SOURCES
CFMTLexer.cpp
FMTIn.cpp
FMTLexer.cpp
FMTOut.cpp
FMTParser.cpp
GDLInterpreter.cpp
GDLLexer.cpp
GDLParser.cpp
GDLTreeParser.cpp
datatypes.cpp #long
convol.cpp #long
convol2.cpp #long
smooth.cpp   #long also
basic_op.cpp
basic_op_new.cpp
getas.cpp
basic_op_add.cpp
basic_op_sub.cpp
basic_op_mult.cpp
basic_op_div.cpp
default_io.cpp
ifmt.cpp
ofmt.cpp
datatypes_minmax.cpp
allix.cpp
arrayindex.cpp
assocdata.cpp
basegdl.cpp
basic_fun.cpp
basic_fun_cl.cpp
basic_fun_jmg.cpp
calendar.cpp
color.cpp
convert2.cpp
dcommon.cpp
dcompiler.cpp
dialog.cpp
dinterpreter.cpp
dnode.cpp
dpro.cpp
dstructdesc.cpp
dstructfactory.cxx
dstructgdl.cpp
dvar.cpp
envt.cpp
extrat.cpp
exists_fun.cpp
fftw.cpp
file.cpp
fmtnode.cpp
gdleventhandler.cpp
gdlexception.cpp
gdlgstream.cpp
dSFMT/dSFMT.c
)

if(WIN32)
set(SOURCES
${SOURCES}
otherdevices/gdlwinstream.cpp
gtdhelper.cpp
otherdevices/devicewin.cpp
)
else(WIN32)
set(SOURCES
${SOURCES}
otherdevices/gdlxstream.cpp
)
endif(WIN32)

set(SOURCES
${SOURCES}
gdljournal.cpp
gdlhelp.cpp
gdlpsstream.cpp
gdlsvgstream.cpp
gdlwidget.cpp
gdlwidgeteventhandler.cpp
gdlwxstream.cpp
gdlzstream.cpp
getfmtast.cpp
graphicsdevice.cpp
basic_pro.cpp
basic_pro_jmg.cpp
brent.cpp
grib.cpp
gsl_fun.cpp
gsl_matrix.cpp
gzstream.hpp
hash.cpp
hdf5_fun.cpp
hdf_fun.cpp
hdf_pro.cpp
image.cpp
initct.cpp
initsysvar.cpp
interpolate.cpp
interpol.cpp
io.cpp
lapack.cpp
least_squares.cpp
libinit.cpp
libinit_ac.cpp
libinit_cl.cpp
libinit_gm.cpp
libinit_jmg.cpp
libinit_mes.cpp
libinit_ng.cpp
libinit_jp.cpp
libinit_exists.cpp
linearprogramming.cpp
list.cpp
magick_cl.cpp
math_fun.cpp
math_fun_ac.cpp
math_fun_gm.cpp
math_fun_jmg.cpp
math_fun_ng.cpp
math_utl.cpp
matrix_cholesky.cpp
matrix_invert.cpp
mpi.cpp
ncdf_att_cl.cpp
ncdf_cl.cpp
ncdf_dim_cl.cpp
ncdf_var_cl.cpp
ncdf4_group.cpp
newprognode.cpp
nullgdl.cpp
objects.cpp
overload.cpp
plotting_axis.cpp
plotting_contour.cpp
plotting_convert_coord.cpp
plotting_cursor.cpp
plotting_device.cpp
plotting_erase.cpp
plotting_image.cpp
plotting_misc.cpp
plotting_oplot.cpp
plotting_plot.cpp
plotting_plots.cpp
plotting_polyfill.cpp
plotting_shade_surf.cpp
plotting_surface.cpp
plotting_windows.cpp
plotting_xyouts.cpp
plotting.cpp
print.cpp
print_tree.cpp
prognode.cpp
prognode_lexpr.cpp
prognodeexpr.cpp
projections.cpp
randomgenerators.cpp
read.cpp
saverestore.cpp
semshm.cpp
sigfpehandler.cpp
sorting.cpp
str.cpp
terminfo.cpp
tiff.cxx
triangulation.cpp
typetraits.cpp
where.cpp
widget.cpp
)
if(USE_SHAPELIB)
set(SOURCES
${SOURCES}
Shapefiles.cpp
)
endif(USE_SHAPELIB)
if(USE_EXPAT)
set (SOURCES
${SOURCES}
sax.cpp
)
endif(USE_EXPAT)
if(HAVE_QHULL)
set (SOURCES
${SOURCES}
qhull.cpp
)
endif(HAVE_QHULL)
if(HAVE_LIBWXWIDGETS AND WIN32)
find_file(WX_RC wx.rc HINTS ${wxWidgets_INCLUDE_DIRS} PATH_SUFFIXES wx/msw)
set(SOURCES
${SOURCES}
${WX_RC}
)
endif(HAVE_LIBWXWIDGETS AND WIN32)

add_subdirectory(antlr)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/antlr ${CMAKE_BINARY_DIR})
link_directories(${LINK_DIRECTORIES})

if(PYTHON_MODULE) #GDL.so
	add_library(gdl SHARED ${SOURCES})
	SET_TARGET_PROPERTIES(gdl PROPERTIES PREFIX "")
	SET_TARGET_PROPERTIES(gdl PROPERTIES OUTPUT_NAME GDL)
	SET_TARGET_PROPERTIES(gdl PROPERTIES NO_SONAME TRUE)
        SET_TARGET_PROPERTIES(gdl PROPERTIES SUFFIX ".so") # e.g. Mac defaults to .dylib which is not looked for by Python
else(PYTHON_MODULE) #GDL.so
	set(SOURCES ${SOURCES} gdl.cpp)
	add_executable(gdl ${SOURCES})
endif(PYTHON_MODULE)

if(USE_OPENMP)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif(USE_OPENMP)

add_dependencies(gdl antlr) # be sure that antlr is built before gdl
target_link_libraries(gdl antlr) # link antlr against gdl

if (MINGW)
target_link_libraries(gdl ws2_32)
endif (MINGW)

target_link_libraries(gdl ${LIBRARIES})
add_definitions(-DHAVE_CONFIG_H)

if(PYTHON_MODULE)
        find_package( PythonInterp ${PYTHONVERSION} REQUIRED )
	execute_process(COMMAND ${PYTHON_EXECUTABLE} -c
		"import site, sys; sys.stdout.write(site.PREFIXES[-1])"
			OUTPUT_VARIABLE PYTHON_PREFIX)
	file( TO_CMAKE_PATH "${PYTHON_PREFIX}" PYTHON_PREFIX)
        execute_process(COMMAND ${PYTHON_EXECUTABLE} -c
		"import site, sys; sys.stdout.write(site.getsitepackages()[-1])"
			OUTPUT_VARIABLE PYTHON_SITE_DIR)
	file( TO_CMAKE_PATH "${PYTHON_SITE_DIR}" PYTHON_SITE_DIR )
	string( REGEX REPLACE "^${PYTHON_PREFIX}/" ""
	       PYTHON_SITE_DIR "${PYTHON_SITE_DIR}")
	install(TARGETS gdl DESTINATION ${PYTHON_SITE_DIR})
else(PYTHON_MODULE)
	install(TARGETS gdl DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
	set_target_properties(gdl PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
endif(PYTHON_MODULE)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/pro/ DESTINATION ${CMAKE_INSTALL_PREFIX}/${GDL_DATA_DIR}/lib
	PATTERN CVS EXCLUDE
	PATTERN checks EXCLUDE
	PATTERN Makefile* EXCLUDE
	PATTERN UrgentNeed.txt EXCLUDE)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/resource DESTINATION ${CMAKE_INSTALL_PREFIX}/${GDL_DATA_DIR}
	PATTERN CVS EXCLUDE
	PATTERN checks EXCLUDE
	PATTERN Makefile* EXCLUDE
	PATTERN UrgentNeed.txt EXCLUDE)
