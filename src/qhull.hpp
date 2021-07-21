/***************************************************************************
                          qhull.hpp  -  GDL header
                             -------------------
    begin                : Jun 09 2021
    copyright            : (C) 2021 by Eloi R. de Linage

***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
 #include <config.h>
#endif

#ifndef QHULL_HPP_
#define QHULL_HPP_

#include "datatypes.hpp"
#include "envt.hpp"

#include <iostream>
#include <complex>
#include <cmath>
#include <cstdio>
#include <vector>

namespace lib {
  void qhull ( EnvT* e);
  BaseGDL* qgrid3_fun ( EnvT* e);
}
#endif
