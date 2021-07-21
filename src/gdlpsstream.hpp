/* *************************************************************************
                          gdlpsstream.hpp  -  graphic stream postscript
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

#ifndef GDLPSSTREAM_HPP_
#define GDLPSSTREAM_HPP_

#include "initsysvar.hpp"
#include "gdlgstream.hpp"

class GDLPSStream: public GDLGStream
{
private:
  int page;
  bool encapsulated;
  long bitsPerPix;
  bool firstTime; //to enable a PS hack on correspondence postscript pixels - plplot position. 
public:
  GDLPSStream( int nx, int ny, int pfont, bool encaps, int color, int bpp):
#ifdef _MSC_VER
    GDLGStream( nx, ny, /*pfont == 1 ? "psttf" :*/ (color==0)?"ps":"psc")
#else
    GDLGStream::GDLGStream( nx, ny, /*pfont == 1 ? "psttf" :*/(color==0)?"ps":"psc")
#endif
  {
    encapsulated = encaps;
    page = 0;
    firstTime=true;
    bitsPerPix=bpp;
  }

  ~GDLPSStream()
  {}
 
  void eop();

  void Init();
  bool PaintImage(unsigned char *idata, PLINT nx, PLINT ny,  DLong *pos, DLong tru, DLong chan);
  //logically close the svg each time an update is made, then rollback to the last graphic section for further graphics.
  void Update(){plstream::cmd(PLESC_EXPOSE, NULL);fprintf(pls->OutFile," S\neop\n");fseek(pls->OutFile,-7, SEEK_END);} 
  float GetPlplotFudge(){return 1;}; //correction factor 
  void DefaultCharSize();
};

#endif
