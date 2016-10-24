/***************************************************************************
 *   Copyright (C) 2016 by Саша Миленковић                                 *
 *   sasa.milenkovic.xyz@gmail.com                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *   ( http://www.gnu.org/licenses/gpl-3.0.en.html )                       *
 *									   *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef TMSMAKER_HPP_INCLUDED
#define TMSMAKER_HPP_INCLUDED

#include <vector>
#include <map>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include "glconstants.hpp"

#include "happyhttp.h"

#define NDEBUG

namespace fs = boost::filesystem;

double roundoff(double, uint);

int write_jpeg_file(const char*, uchar* );
int jpg_compressed_bytes_2_uncompressed_bytes();
int UpNullDownJpeg2Jpeg(const char*);

void updateGlobalConstants();

void createDirs(char const* chdirtms=NULL);

void makeCacheLevel(int);
void makeTmsBase();

void makeTmsComplete();

void makeTmsSampleOL2_strict();
void makeTmsSampleOL2_amended();
void makeTmsSampleOL3();
void makeWmsSampleOL2();
void makeWmsSampleOL3();
void makeWmsSampleArcGisJSApi();
void makeWmsPhp();
void makeWmsCapabilites();
void makeExceptionXmls();

void makeHttpTest();

#endif // TMSMAKER_HPP_INCLUDED

