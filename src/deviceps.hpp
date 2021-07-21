/* *************************************************************************
                          deviceps.hpp  -  postscript device
                             -------------------
    begin                : July 22 2002
    copyright            : (C) 2002 by Marc Schellens
    email                : m_schellens@users.sf.net
 ***************************************************************************/

/* *************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef DEVICEPS_HPP_
#define DEVICEPS_HPP_

#include "gdlpsstream.hpp"
#include "plotting.hpp" // get_axis_crange for TV()
#include "initsysvar.hpp"
#include <gsl/gsl_const_mksa.h> // GSL_CONST_MKSA_INCH

#include "objects.hpp"


  static const float CM2IN = .01 / GSL_CONST_MKSA_INCH;
  static const float in2cm = GSL_CONST_MKSA_INCH*100;
  static const PLFLT DPI = 72.0 ; //in dpi;
  static const PLFLT DPICM = 72.0/2.54 ; //dpi/cm;
  static const float RESOL = 1000.0;
  static const PLFLT PlplotInternalPageRatioXoverY=4./3.; //Some machines do not know PRIVATE values stored in plplotP.h 4/3=PlplotInternalPageRatioXoverY=float(PIXELS_X)/float(PIXELS_Y)

class DevicePS: public GraphicsDevice
{
  std::string      fileName;
  GDLPSStream*     actStream;
  float            XPageSize;
  float            YPageSize;
  float            XOffset;
  float            YOffset;
  int              color;
  int              decomposed; // false -> use color table
  bool	           orient_portrait; 
  bool             encapsulated;
  int              bitsPerPix;
  float	           scale;

  GDLStream  *psUnit;

  void InitStream()
  {
    delete actStream;

    DLongGDL* pMulti = SysVar::GetPMulti();
    DLong nx = (*pMulti)[ 1];
    DLong ny = (*pMulti)[ 2];

    if( nx <= 0) nx = 1;
    if( ny <= 0) ny = 1;
    actStream = new GDLPSStream( nx, ny, (int)SysVar::GetPFont(), encapsulated, color, bitsPerPix);

    actStream->sfnam( fileName.c_str());

    // trying to solve bug report 3611898
    // AC 29-Avril-2013: the way I found to link GDLPSStream* and GDLStream*
    DLong lun=GetLUN();
    psUnit = &fileUnits[ lun-1];
    psUnit->Open(fileName,fstream::out,false,false,false,defaultStreamWidth,false,false);
    (*static_cast<DLongGDL*>( dStruct->GetTag(dStruct->Desc()->TagIndex("UNIT"))))[0]=lun;

    // zeroing offsets (xleng and yleng are the default ones but they need to be specified 
    // for the offsets to be taken into account by spage(), works with plplot >= 5.9.9)
    PLINT XSIZE=ceil(XPageSize*DPICM);
    PLINT YSIZE=ceil(YPageSize*DPICM);
    PLINT XOFF=ceil(XOffset*DPICM);
    PLINT YOFF=ceil(YOffset*DPICM);

    // as setting the offsets and sizes with plPlot is (extremely) tricky, and some of these setting
    // are hardcoded into plplot (like EPS header, and offsets in older versions of plplot)
    // here we play only with the aspect ratio 

    // plot orientation
    //std::cout  << "orientation : " << orient_portrait<< '\n';
    if (orient_portrait) { //X size will be OK, Y size must be scaled 
     actStream->setopt( "portrait",NULL);
     actStream->sdidev( PL_NOTSET, PlplotInternalPageRatioXoverY, PL_NOTSET, PL_NOTSET ); //only OK if page ratio is 540x720 
     actStream->spage(DPI, DPI, XSIZE, YSIZE, YOFF, XOFF);

    } else {
     actStream->spage(DPI, DPI, YSIZE, XSIZE, YOFF-YSIZE, XOFF); //invert axes, displace as does IDL..
     actStream->sdiori(2);
    }

    // no pause on destruction
    actStream->spause( false);

    // extended fonts
    actStream->fontld( 1);
    
    PLINT r[ctSize], g[ctSize], b[ctSize];
    actCT.Get( r, g, b);
    actStream->SetColorMap0( r, g, b, ctSize);
    actStream->SetColorMap1( r, g, b, ctSize);
    // default: black+white (IDL behaviour)
    //force TTF fonts as scaling of hershey fonts will not be good 
    short font=((int)SysVar::GetPFont()>-1)?1:0;
//    string what="text="+i2s(font)+",color="+i2s(color);
    string what="text=1,color="+i2s(color);
    actStream->setopt( "drvopt",what.c_str());
    actStream->scolbg(255,255,255); // start with a white background

    actStream->Init();
    
    // need to be called initially. permit to fix things
    actStream->ssub(1,1);
    actStream->adv(0);
    // load font
    actStream->font( 1);
    actStream->vpor(0,1,0,1);
    actStream->wind(0,1,0,1);
    actStream->DefaultCharSize();
  }
    
private:
  void epsHacks()
  {
    // using namespace std;
    //PLPLOT outputs a strange boundingbox; this hack directly edits the eps file.  
    //if the plplot bug ever gets fixed, this hack won't be needed.
    char *bb;
    FILE *feps;
    const size_t buflen=2048;//largely sufficient
    char buffer[buflen]; 
    int cnt;
    feps=fopen(fileName.c_str(), "r");
    cnt=fread(buffer,sizeof(char),buflen,feps);

    //read original boundingbox
    bb = strstr(buffer, "%%BoundingBox:");
    if (bb==NULL) {
      Warning("Warning: failed to read temporary PostScript file.");
      fclose(feps);
      return;
    }
    
    string sbuff = string(buffer);
    stringstream searchstr,replstr;
    size_t pos;
    int extralen=0;

// Do not change bonding box. It is good now.
//    int offx, offy, width, height;
//    bb += 15;
//    sscanf(bb, "%i %i %i %i", &offx, &offy, &width, &height);
//    float hsize = XPageSize*CM2IN*DPI*scale;
//    float vsize = YPageSize*CM2IN*DPI*scale;
//    float newwidth = (width - offx), newheight = (height - offy);
//    float hscale = (orient_portrait ? hsize : vsize)/newwidth/5.0;
//    float vscale = (orient_portrait ? vsize : hsize)/newheight/5.0;
////    hscale = min(hscale,vscale)*0.98;
//    hscale = min(hscale,vscale);
//    vscale = hscale;
//    float hoff = -5.*offx*hscale + ((orient_portrait ? hsize : vsize) - 5.0*hscale*newwidth)*0.5;
//    float voff = -5.*offy*vscale + ((orient_portrait ? vsize : hsize) - 5.0*vscale*newheight)*0.5;
//
//    //replace with a more sensible boundingbox
//    searchstr << "BoundingBox: " << offx << " " << offy << " " << width << " " << height;
//    replstr << "BoundingBox: 0 0 " << floor((orient_portrait ? hsize : vsize)+0.5) << " " << floor((orient_portrait ? vsize : hsize)+0.5);
//      pos = sbuff.find(searchstr.str());
//    if (pos != string::npos) {
//      sbuff.replace(pos,searchstr.str().length(),replstr.str()); 
//      extralen = replstr.str().length()-searchstr.str().length();
//    }

    //replace the values of linecap and linejoin to nice round butts (sic!) more pleasing to the eye.
    searchstr.str("");
    searchstr << "0 setlinecap" << '\n' << "    0 setlinejoin";
    replstr.str("");
    replstr << "1 setlinecap" << '\n' << "    1 setlinejoin";
    pos = sbuff.find(searchstr.str());
    if (pos != string::npos) {
      sbuff.replace(pos,searchstr.str().length(),replstr.str()); 
      extralen = extralen + replstr.str().length()-searchstr.str().length();
    }
    
    //add landscape
    if (!orient_portrait) {
    searchstr.str("%%Page: 1 1");
    replstr.str("");
    replstr << "%%Page: 1 1" << '\n' << "%%PageOrientation: Landscape" << '\n';
    pos = sbuff.find(searchstr.str());
    if (pos != string::npos) {
      sbuff.replace(pos,searchstr.str().length(),replstr.str()); 
      extralen = extralen + replstr.str().length()-searchstr.str().length();
    }
    }

    //open temp file
    FILE *fp = tmpfile(); // this creates a file which should be deleted automaticaly when it is closed
    FILEGuard fpGuard( fp, fclose);
    if (fp == NULL) { 
      Warning("Warning: failed to create temporary PostScript file.");
      fclose(feps);
      return;
    }

    // write the first buflen to temp file
    fwrite(sbuff.c_str(), 1, buflen+extralen, fp); 
    
    // read the rest of feps and write to temp file
    while (true)
      {
    	cnt = fread(&buffer, 1, buflen, feps);
    	if (!cnt) break;
        if (fwrite(&buffer, 1, cnt, fp) < cnt)
    	  {
    	    Warning("Warning: failed to write to temporary file");
    	  }
      }
    fclose(feps);

    // copy temp file to fileName
    rewind(fp);
    FILE *fp_plplot = fopen(fileName.c_str(), "w");
    FILEGuard fp_plplotGuard( fp_plplot, fclose);
    if (fp_plplot == NULL) {
      Warning("Warning: failed to open plPlot-generated file");
      return;
    }
    while (true)
      {
    	cnt = fread(&buffer, 1, buflen, fp);
    	if (!cnt) break;
        if (fwrite(&buffer, 1, cnt, fp_plplot) < cnt)
    	  {
    	    Warning("Warning: failed to overwrite the plPlot-generated file");
    	  }
      }

  }

public:
  DevicePS(): GraphicsDevice(), fileName( "gdl.ps"), actStream( NULL),
    XPageSize(17.78), YPageSize(12.7), XOffset(1.905),YOffset(12.7),  //IDL default for offests: 54 pts /X and 360 pts/Y
    color(0), decomposed( 0), encapsulated(false), scale(1.), orient_portrait(true), bitsPerPix(8)
  {
    name = "PS";

    DLongGDL origin( dimension( 2));
    DLongGDL zoom( dimension( 2));
    zoom[0] = 1;
    zoom[1] = 1;

    dStruct = new DStructGDL( "!DEVICE");
    dStruct->InitTag("NAME",       DStringGDL( name)); 
    dStruct->InitTag("X_SIZE",     DLongGDL( XPageSize*scale*RESOL)); 
    dStruct->InitTag("Y_SIZE",     DLongGDL( YPageSize*scale*RESOL));
    dStruct->InitTag("X_VSIZE",    DLongGDL( XPageSize*scale*RESOL));
    dStruct->InitTag("Y_VSIZE",    DLongGDL( YPageSize*scale*RESOL));
    dStruct->InitTag("X_CH_SIZE",  DLongGDL( 222));
    dStruct->InitTag("Y_CH_SIZE",  DLongGDL( 352));
    dStruct->InitTag("X_PX_CM",    DFloatGDL( RESOL)); 
    dStruct->InitTag("Y_PX_CM",    DFloatGDL( RESOL)); 
    dStruct->InitTag("N_COLORS",   DLongGDL( 256)); 
    dStruct->InitTag("TABLE_SIZE", DLongGDL( 256)); 
    dStruct->InitTag("FILL_DIST",  DLongGDL( 1));
    dStruct->InitTag("WINDOW",     DLongGDL( -1)); 
    dStruct->InitTag("UNIT",       DLongGDL( 0)); 
    dStruct->InitTag("FLAGS",      DLongGDL( 266791)); //266807 if color, 266791 if monochrome.
    dStruct->InitTag("ORIGIN",     origin); 
    dStruct->InitTag("ZOOM",       zoom);

    SetPortrait();
  }
  
  ~DevicePS()
  {
    delete actStream;
  }

  GDLGStream* GetStream( bool open=true)
  {
    if( actStream == NULL) 
      {
	if( !open) return NULL;
	InitStream();
      }
    return actStream;
  }

  bool SetFileName( const std::string& f)
  {
    fileName = f;
    return true;
  }

  bool CloseFile()
  {
    // trying to solve bug report 3611898
    // this is needed to decrement Lun number ...
    (*static_cast<DLongGDL*>( dStruct->GetTag(dStruct->Desc()->TagIndex("UNIT"))))[0]=0;
    if (actStream != NULL)
    {
      psUnit->Close();
      psUnit->Free();
      psUnit=NULL;

      delete actStream;
      actStream = NULL;
      //always : if (encapsulated) 
       epsHacks(); // needs to be called after the plPlot-generated file is closed
    }
    return true;
  }

  bool SetXOffset( const float xo) // xo [cm]
  {
    XOffset=xo;
    return true;
  }

  bool SetYOffset( const float yo) // yo [cm]
  {
    YOffset=yo;
    return true;
  }

  bool SetXPageSize( const float xs) // xs [cm]
  {
    XPageSize=xs;
    (*static_cast<DLongGDL*>(dStruct->GetTag(dStruct->Desc()->TagIndex("X_SIZE"))))[0] 
      = DLong(floor(0.5+
        xs * (*static_cast<DFloatGDL*>(dStruct->GetTag(dStruct->Desc()->TagIndex("X_PX_CM"))))[0]
      ));
   (*static_cast<DLongGDL*>(dStruct->GetTag(dStruct->Desc()->TagIndex("X_VSIZE"))))[0]
      = DLong(floor(0.5+
        xs * (*static_cast<DFloatGDL*>(dStruct->GetTag(dStruct->Desc()->TagIndex("X_PX_CM"))))[0]
      ));
    return true;
  }

  bool SetYPageSize( const float ys) // ys [cm]
  {
    YPageSize=ys;
    (*static_cast<DLongGDL*>(dStruct->GetTag(dStruct->Desc()->TagIndex("Y_SIZE"))))[0] 
      = DLong(floor(0.5+
        ys * (*static_cast<DFloatGDL*>(dStruct->GetTag(dStruct->Desc()->TagIndex("Y_PX_CM"))))[0]
      ));
    (*static_cast<DLongGDL*>(dStruct->GetTag(dStruct->Desc()->TagIndex("Y_VSIZE"))))[0]
      = DLong(floor(0.5+
        ys * (*static_cast<DFloatGDL*>(dStruct->GetTag(dStruct->Desc()->TagIndex("Y_PX_CM"))))[0]
      ));
    return true;
  }

  bool SetColor(const long hascolor)
  {
    if (hascolor==1) color=1; else color=0;
      if (hascolor==1) 
      {
        DLong FLAG=(*static_cast<DLongGDL*>( dStruct->GetTag(dStruct->Desc()->TagIndex("FLAGS"))))[0];
        (*static_cast<DLongGDL*>( dStruct->GetTag(dStruct->Desc()->TagIndex("FLAGS"))))[0]=FLAG|16; //set colored device
      } else {
      DLong FLAG=(*static_cast<DLongGDL*>( dStruct->GetTag(dStruct->Desc()->TagIndex("FLAGS"))))[0];
        (*static_cast<DLongGDL*>( dStruct->GetTag(dStruct->Desc()->TagIndex("FLAGS"))))[0]=FLAG&(~16); //set monochrome device
      }
      //trick, to be repeated in Decomposed()
      DLong FLAG=(*static_cast<DLongGDL*>( dStruct->GetTag(dStruct->Desc()->TagIndex("FLAGS"))))[0];
      if (decomposed==1 && color==1) (*static_cast<DLongGDL*>(SysVar::D()->GetTag(SysVar::D()->Desc()->TagIndex("FLAGS"), 0)))[0]= FLAG&(~512); //remove flag 'printer' since logic does not work with ps drive
      else (*static_cast<DLongGDL*>(SysVar::D()->GetTag(SysVar::D()->Desc()->TagIndex("FLAGS"), 0)))[0]= FLAG|(512); //set Flag printer
    return true;
  }

  bool SetPortrait()
  {
      // no need to update !D
    orient_portrait = true;
//    nb: IDL defaults to:
//    SetXPageSize(7 * in2cm);
//    SetYPageSize(5 * in2cm);
//    SetXOffset(.75 * in2cm);
//    SetYOffset(5 * in2cm); 
    return true;
  }

  bool SetLandscape()
  {
      // no need to update !D
    orient_portrait = false;
//    SetXPageSize(9.5 * in2cm);
//    SetYPageSize(7.0 * in2cm);
//    SetXOffset(.75 * in2cm);
//    SetYOffset(10.25 * in2cm);
    return true;
  }

  bool SetScale(float value)
  {
      //no effect for postscript in IDL up to 8 (?)
    scale = value;
    return true;
  }

  bool SetEncapsulated(bool val)
  {
    encapsulated = val;
    return true;
  }
  
  bool SetBPP(const int val)
  {
    int bpp = max(min(8,val),1);
    if (bpp > 4) bpp = 8;
    else if (bpp > 2) bpp = 4;
    bitsPerPix = bpp;
    return true;
  }
  
  bool Decomposed( bool value)           
  {   
    decomposed = value;
    if (decomposed==1) (*static_cast<DLongGDL*>( dStruct->GetTag(dStruct->Desc()->TagIndex("N_COLORS"))))[0]=256*256*256;
    else (*static_cast<DLongGDL*>( dStruct->GetTag(dStruct->Desc()->TagIndex("N_COLORS"))))[0]=256;
    DLong FLAG=(*static_cast<DLongGDL*>(SysVar::D()->GetTag(SysVar::D()->Desc()->TagIndex("FLAGS"), 0)))[0];
    //trick, to be repeated in SetColor(). To compensate a problem in ps driver. Other possibilities: use only the psc driver and do the black & white directly ourselves. 
    if (decomposed==1 && color==1) { (*static_cast<DLongGDL*>(SysVar::D()->GetTag(SysVar::D()->Desc()->TagIndex("FLAGS"), 0)))[0]= FLAG&(~512); //remove flag 'printer' since logic does not work with ps drive
    } else (*static_cast<DLongGDL*>(SysVar::D()->GetTag(SysVar::D()->Desc()->TagIndex("FLAGS"), 0)))[0]= FLAG|(512); //set Flag printer
    return true;
  }

  DLong GetDecomposed()        
  {
    return decomposed;  
  }

  DIntGDL* GetPageSize()
  {
    DIntGDL* res;
    res = new DIntGDL(2, BaseGDL::NOZERO);
    (*res)[0]= XPageSize;
    (*res)[1]= YPageSize;
    return res;
  }
};

#endif
