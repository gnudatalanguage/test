/***************************************************************************
                  libinit_jmg.cpp  -  initialization of GDL library routines
                             -------------------
    begin                : 2004
    copyright            : (C) 2004 by Joel Gales
    email                : jomoga@users.sourceforge.net 
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "includefirst.hpp"

#include <string>
#include <fstream>

#include "envt.hpp"

#include "basic_fun_jmg.hpp"
#include "math_fun_jmg.hpp"
#include "basic_pro_jmg.hpp"

#include "gsl_fun.hpp"
#include "interpolate.hpp"
#include "interpol.hpp"
#include "image.hpp"
#include "plotting.hpp"
#include "projections.hpp"


#if defined(USE_HDF)
#include "hdf_fun.hpp"
#include "hdf_pro.hpp"
#endif

#if defined(USE_FFTW)
#include "fftw.hpp"
#endif

#if defined(USE_EIGEN)
#include "matrix_cholesky.hpp"
#include "least_squares.hpp"
#endif

#include "matrix_invert.hpp"
#include "triangulation.hpp"

#ifdef HAVE_QHULL
#include "qhull.hpp"
#endif

using namespace std;

void LibInit_jmg()
{
  const char KLISTEND[] = "";

  // the following by Joel Gales <jomoga@users.sourceforge.net>
  const string sizeKey[]={"L64","DIMENSIONS","FILE_LUN","FILE_OFFSET",
			  "N_DIMENSIONS","N_ELEMENTS",
			  "SNAME","STRUCTURE","TNAME","TYPE",KLISTEND};
  new DLibFunRetNew(lib::size_fun,string("SIZE"),1,sizeKey);

  new DLibFunRetNew(lib::fstat_fun,string("FSTAT"),1);

  const string routine_namesKey[]={"LEVEL","VARIABLES","FETCH","ARG_NAME",
				   "STORE","S_FUNCTIONS","S_PROCEDURES",
				   KLISTEND};
  new DLibFun(lib::routine_names_value,string("ROUTINE_NAMES"),-1,routine_namesKey);
  
  // the 2 following by Alain C. and Levan L., Summer 2014
  const string isaKey[]={"ARRAY","FILE","NULL","NUMBER","SCALAR","BOOLEAN",
			 "COMPLEX","FLOAT","INTEGER","STRING",KLISTEND};
  new DLibFunRetNew(lib::isa_fun,string("ISA"),2,isaKey);

  new DLibFunRetNew(lib::typename_fun,string("TYPENAME"),1);

  // the following (Chol and Invert) by Alain C. and Nodar K., Summer 2013
  // Since we have a difference between the IDL way and the Eigen way
  // we (temporary) remove these 2 codes in the CVS of GDL
  // Help welcome. 
#if defined(USE_EIGEN)
  const string cholKey[]={"DOUBLE",KLISTEND};
  new DLibPro(lib::choldc_pro,string("CHOLDC"),2,cholKey);
  new DLibFunRetNew(lib::cholsol_fun,string("CHOLSOL"),3,cholKey);

  //
  const string lacholdcKey[]={"DOUBLE","STATUS","UPPER",KLISTEND};
  new DLibPro(lib::la_choldc_pro,string("LA_CHOLDC"),1,lacholdcKey);
  const string lacholsolKey[]={"DOUBLE","STATUS",KLISTEND};
  new DLibFunRetNew(lib::la_cholsol_fun,string("LA_CHOLSOL"),2,lacholsolKey);

  // Ilia N. & Alain C., Summer 2015
  const string laleastsquaresKey[]={"DOUBLE","METHOD","RANK","RCONDITION","RESIDUAL","STATUS",KLISTEND};
  new DLibFunRetNew(lib::la_least_squares_fun,string("LA_LEAST_SQUARES"),2,laleastsquaresKey);

#endif

#if defined(HAVE_LIBGSL) && defined(HAVE_LIBGSLCBLAS)
  
  const string invertKey[]={"DOUBLE","GSL","EIGEN",KLISTEND};
  new DLibFunRetNew(lib::AC_invert_fun,string("INVERT"),2,invertKey);

  // if FFTw not available, FFT in the GSL used (slower)
  const string fftKey[]={"DOUBLE","INVERSE","OVERWRITE","DIMENSION","CENTER",KLISTEND};
#if defined(USE_FFTW)
  new DLibFun(lib::fftw_fun,string("FFT"),2,fftKey);
#else
  new DLibFun(lib::fft_fun,string("FFT"),2,fftKey);
#endif

  const string randomKey[]={"DOUBLE","GAMMA","LONG","NORMAL",
			    "BINOMIAL","POISSON","UNIFORM","ULONG","RAN1",KLISTEND}; //note WARNING: RAN1 has a special meaning for GDL, see code.
  new DLibFunRetNew(lib::random_fun,string("RANDOMU"),MAXRANK+1,randomKey);
  new DLibFunRetNew(lib::random_fun,string("RANDOMN"),MAXRANK+1,randomKey);

  const string checkmathKey[]={"MASK","NOCLEAR","PRINT",KLISTEND};
  new DLibFunRetNew(lib::check_math_fun,string("CHECK_MATH"),2,checkmathKey);

  const string histogramKey[]={"BINSIZE","INPUT","MAX","MIN","NBINS",
			       "OMAX","OMIN","REVERSE_INDICES",
			       "LOCATIONS","NAN",KLISTEND};
  const string histogramWarnKey[]={"L64",KLISTEND};
  new DLibFunRetNew(lib::histogram_fun,string("HISTOGRAM"),1,histogramKey,histogramWarnKey);
  
  const string interpolKey[]={ "LSQUADRATIC","NAN", "QUADRATIC", "SPLINE" ,KLISTEND};
  new DLibFunRetNew(lib::interpol_fun,string("INTERPOL"),3,interpolKey);
  
  const string interpolateKey[]={"CUBIC","DOUBLE","GRID","MISSING","NEAREST_NEIGHBOUR",KLISTEND};
  new DLibFunRetNew(lib::interpolate_fun,string("INTERPOLATE"),4,interpolateKey);

  const string la_triredKey[]={"DOUBLE","UPPER",KLISTEND};
  new DLibPro(lib::la_trired_pro,string("LA_TRIRED"),3,la_triredKey);
#endif

  const string macharKey[]={"DOUBLE",KLISTEND};
  new DLibFunRetNew(lib::machar_fun,string("MACHAR"),0,macharKey);

#if defined(USE_LIBPROJ)
  const string map_proj_forwardKey[]={"MAP_STRUCTURE","RADIANS","POLYGONS","POLYLINES","CONNECTIVITY","FILL",KLISTEND};  //WARNING FIXED ORDER for GetMapAsMapStructureKeyword()
  new DLibFunRetNew(lib::map_proj_forward_fun,
	      string("MAP_PROJ_FORWARD"),2,map_proj_forwardKey,NULL);

  const string map_proj_inverseKey[]={"MAP_STRUCTURE","RADIANS",KLISTEND}; //WARNING FIXED ORDER for GetMapAsMapStructureKeyword()
  new DLibFunRetNew(lib::map_proj_inverse_fun,
	      string("MAP_PROJ_INVERSE"),2,map_proj_inverseKey);
//dummy functions for compatibility support of GCTP projections 
  new DLibPro(lib::map_proj_gctp_forinit,string("MAP_PROJ_GCTP_FORINIT"),4);
  new DLibPro(lib::map_proj_gctp_revinit,string("MAP_PROJ_GCTP_REVINIT"),4);

#endif
  


  const string convert_coordKey[]={"DATA","DEVICE","NORMAL","T3D","DOUBLE",
				   "TO_DATA","TO_DEVICE","TO_NORMAL",KLISTEND};
  new DLibFunRetNew(lib::convert_coord,string("CONVERT_COORD"),3,convert_coordKey);


  const string finiteKey[]={"INFINITY","NAN","SIGN",KLISTEND};
  new DLibFunRetNew(lib::finite_fun,string("FINITE"),1,finiteKey);

  const string radonKey[]={"BACKPROJECT","DOUBLE","DRHO","DX","DY",
			   "GRAY","LINEAR","NRHO","NTHETA","NX","NY",
			   "RHO","RMIN","THETA","XMIN","YMIN",KLISTEND};
  new DLibFunRetNew(lib::radon_fun,string("RADON"),1,radonKey);
  

  const string grid_inputKey[]={"SPHERE", "POLAR", "DEGREES", "DUPLICATES", "EPSILON", "EXCLUDE", KLISTEND};
  new DLibPro(lib::grid_input,string("GRID_INPUT"),6,grid_inputKey);
  
  const string triangulateKey[]={"CONNECTIVITY", "SPHERE", "DEGREES", "FVALUE","REPEATS", "TOLERANCE",KLISTEND};
  new DLibPro(lib::GDL_Triangulate,string("TRIANGULATE"),4,triangulateKey);

//to be written and do not forget to uncomment QHULL in CMakeLists and config.h.cmake  
#ifdef HAVE_QHULL

  const string qhullKey[]={"BOUNDS", "CONNECTIVITY", "DELAUNAY", "SPHERE", "VDIAGRAM" ,"VNORMALS", "VVERTICES", KLISTEND};
  new DLibPro(lib::qhull,string("QHULL"),8,qhullKey);


  const string qgrid3Key[]={"DELTA", "DIMENSION", "MISSING", "START", KLISTEND};
  new DLibFunRetNew(lib::qgrid3_fun,string("QGRID3"),5,qgrid3Key);
#endif
  
  const string trigridKey[]={"DEGREES","NX","NY","SPHERE","XGRID","XOUT","YGRID","YOUT","MAX_VALUE","MIN_VALUE","MISSING","INPUT",KLISTEND};
  const string trigridWarnKey[]={"EXTRAPOLATE","QUINTIC",KLISTEND};
  new DLibFunRetNew(lib::trigrid_fun,string("TRIGRID"),6,trigridKey,trigridWarnKey);

  const string poly_2dKey[]={"CUBIC","MISSING",KLISTEND};
  new DLibFunRetNew(lib::poly_2d_fun,string("POLY_2D"),6,poly_2dKey);

  const string make_arrayKey[]={"DIMENSION", "INCREMENT", "INDEX", "NOZERO",
                                 "SIZE", "START", "TYPE", "VALUE", "BOOLEAN",
                                 "BYTE", "COMPLEX", "DCOMPLEX", "DOUBLE",
                                 "FLOAT", "INTEGER", "L64", "LONG", "OBJ",
                                 "PTR", "STRING", "UINT", "UL64", "ULONG", KLISTEND};
  new DLibFunRetNew(lib::make_array,string("MAKE_ARRAY"),MAXRANK,make_arrayKey);

  const string reformKey[]={"OVERWRITE",KLISTEND};
  new DLibFun(lib::reform,string("REFORM"),MAXRANK+1,reformKey);

  new DLibPro(lib::point_lun,string("POINT_LUN"),2);
  new DLibPro(lib::truncate_lun,string("TRUNCATE_LUN"),-1);

  const string skip_or_copy_lunKey[]={"EOF","LINES","TRANSFER_COUNT",KLISTEND};
  new DLibPro(lib::skip_lun,string("SKIP_LUN"),2,skip_or_copy_lunKey);
  new DLibPro(lib::copy_lun,string("COPY_LUN"),3,skip_or_copy_lunKey);
  
  const string linkimageKey[]={"FUNCT", "KEYWORDS", "MAX_ARGS", "MIN_ARGS", "DEFAULT", KLISTEND}; //DEFAULT is VMS obsolete
  new DLibPro( lib::linkimage, string("LINKIMAGE"), 4, linkimageKey, NULL, 2 );
  const string unlinkimageKey[]={"FORCE", KLISTEND};
  new DLibPro( lib::unlinkimage, string("UNLINKIMAGE"), 1, unlinkimageKey );
  const string unlinksymbolKey[]={"FUNCT", KLISTEND};
  new DLibPro( lib::unlinksymbol, string("UNLINKSYMBOL"), 2, unlinksymbolKey, NULL, 2 );

  new DLibPro(lib::wait_pro,string("WAIT"),1);

#if defined(USE_HDF)
  new DLibFunRetNew(lib::hdf_ishdf,string("HDF_ISHDF"),1);
  const string hdf_openKey[]={"READ","RDWR","CREATE","ALL","NUM_DD","WRITE",KLISTEND};
  new DLibFunRetNew(lib::hdf_open_fun,string("HDF_OPEN"),2,hdf_openKey);

  new DLibFunRetNew(lib::hdf_vg_getid_fun,string("HDF_VG_GETID"),2);

  const string hdf_vg_attachKey[]={"READ","WRITE",KLISTEND};
  new DLibFunRetNew(lib::hdf_vg_attach_fun,string("HDF_VG_ATTACH"),2,
	      hdf_vg_attachKey);

  const string hdf_vd_attachKey[]={"READ","WRITE",KLISTEND};
  new DLibFunRetNew(lib::hdf_vd_attach_fun,string("HDF_VD_ATTACH"),2,
	      hdf_vd_attachKey);
  new DLibFunRetNew(lib::hdf_vd_find_fun,string("HDF_VD_FIND"),2);

  const string hdf_vd_readKey[]={"FIELDS","NRECORDS","FULL_INTERLACE",
				 "NO_INTERLACE",KLISTEND};
  new DLibFunRetNew(lib::hdf_vd_read_fun,string("HDF_VD_READ"),2,hdf_vd_readKey);

  const string hdf_vg_getinfoKey[]={"CLASS","NAME","NENTRIES","REF","TAG",
				    KLISTEND};
  new DLibPro(lib::hdf_vg_getinfo_pro,string("HDF_VG_GETINFO"),1,
	      hdf_vg_getinfoKey);

  const string hdf_vd_getKey[]={"CLASS","NAME","COUNT","REF","TAG",KLISTEND};
  new DLibPro(lib::hdf_vd_get_pro,string("HDF_VD_GET"),1,
	      hdf_vd_getKey);


  new DLibPro(lib::hdf_vg_gettrs_pro,string("HDF_VG_GETTRS"),3);
  new DLibPro(lib::hdf_vg_detach_pro,string("HDF_VG_DETACH"),1);

  new DLibPro(lib::hdf_vd_detach_pro,string("HDF_VD_DETACH"),1);

  const string hdf_sd_createKey[]={"BYTE","DFNT_INT8","DFNT_UINT8",
				   "SHORT","INT","DFNT_INT16","DFNT_UINT16",
				   "LONG","DFNT_INT32","DFNT_UINT32",
				   "FLOAT","DFNT_FLOAT32",
				   "DOUBLE","DFNT_FLOAT64",
				   "STRING","DFNT_CHAR","HDF_TYPE",KLISTEND};
  new DLibFunRetNew(lib::hdf_sd_create_fun,string("HDF_SD_CREATE"),3,
	      hdf_sd_createKey);

  const string hdf_sd_startKey[]={"READ","RDWR","CREATE",KLISTEND};
  new DLibFunRetNew(lib::hdf_sd_start_fun,string("HDF_SD_START"),2,hdf_sd_startKey);
  new DLibFunRetNew(lib::hdf_sd_nametoindex_fun,string("HDF_SD_NAMETOINDEX"),2);
  new DLibFunRetNew(lib::hdf_sd_attrfind_fun,string("HDF_SD_ATTRFIND"),2);
  new DLibFunRetNew(lib::hdf_sd_select_fun,string("HDF_SD_SELECT"),2);

  const string hdf_sd_getdataKey[]={"START","STRIDE","COUNT",KLISTEND};
  new DLibPro(lib::hdf_sd_getdata_pro,string("HDF_SD_GETDATA"),2,
	      hdf_sd_getdataKey);
  const string hdf_sd_adddataKey[]={"START","STRIDE","COUNT",KLISTEND};
  new DLibPro(lib::hdf_sd_adddata_pro,string("HDF_SD_ADDDATA"),2,
	      hdf_sd_adddataKey);
  new DLibPro(lib::hdf_sd_fileinfo_pro,string("HDF_SD_FILEINFO"),3);

  const string hdf_sd_getinfoKey[]={"DIMS","HDF_TYPE","NAME",
				    "NATTS","NDIMS","TYPE",
				    "LABEL","UNIT","FORMAT","COORDSYS",
				    KLISTEND};
  new DLibPro(lib::hdf_sd_getinfo_pro,string("HDF_SD_GETINFO"),1,
	      hdf_sd_getinfoKey);

  const string hdf_sd_attrinfoKey[]={"COUNT","DATA","HDF_TYPE","NAME",
				     "TYPE",KLISTEND};
  new DLibPro(lib::hdf_sd_attrinfo_pro,string("HDF_SD_ATTRINFO"),2,
	      hdf_sd_attrinfoKey);

  new DLibPro(lib::hdf_sd_endaccess_pro,string("HDF_SD_ENDACCESS"),1);
  new DLibPro(lib::hdf_sd_end_pro,string("HDF_SD_END"),1);
  new DLibPro(lib::hdf_close_pro,string("HDF_CLOSE"),1);
  
  new DLibFunRetNew(lib::hdf_sd_dimgetid_fun, string("HDF_SD_DIMGETID"), 2);
  const string hdf_sd_dimgetKey[] = {
    "NAME", "NATTR", "SCALE", "COUNT", KLISTEND
  };
  const string hdf_sd_dimgetWarnKey[] = {
    "COMPATIBILITY", "FORMAT", "LABEL", "TYPE", "UNIT", KLISTEND
  };
  new DLibPro(lib::hdf_sd_dimget_pro, string("HDF_SD_DIMGET"), 1, 
    hdf_sd_dimgetKey, hdf_sd_dimgetWarnKey);
 
#endif

  const string tvKey[]={"TRUE","NORMAL","CHANNEL","XSIZE","YSIZE","ORDER","DEVICE","DATA","T3D","Z","CENTIMETERS","INCHES",KLISTEND};
  const string tvWarnKey[]={"WORDS",KLISTEND};
  new DLibPro(lib::tv_image,string("TV"),4,tvKey,tvWarnKey);

  const string tvrdKey[]={"CHANNEL","ORDER","TRUE","WORDS",KLISTEND};
  new DLibFunRetNew(lib::tvrd,string("TVRD"),5,tvrdKey);

  const string loadctKey[]={"GET_NAMES","BOTTOM","NCOLORS","RGB_TABLE","SILENT",KLISTEND};
  const string loadctWarnKey[]={"FILE",KLISTEND};
  new DLibPro(lib::loadct,string("LOADCT_INTERNALGDL"),1,loadctKey,loadctWarnKey);

  // call_external (by Christoph Fuchs)
  const string call_externalKey[] = {"VALUE", "ALL_VALUE", "RETURN_TYPE",
               "B_VALUE", "I_VALUE", "L_VALUE", "F_VALUE", "D_VALUE",
               "UI_VALUE", "UL_VALUE", "L64_VALUE", "UL64_VALUE", "S_VALUE",
               "UNLOAD", "ALL_GDL", "STRUCT_ALIGN_BYTES"
    , "DEFAULT", "PORTABLE", "VAX_FLOAT" // obsoleted VMS
    , KLISTEND };
  new DLibFunRetNew(lib::call_external, string("CALL_EXTERNAL"), -1, call_externalKey);
}
