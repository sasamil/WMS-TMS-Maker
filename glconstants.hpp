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

 #include <string>

typedef unsigned char uchar;
typedef unsigned int uint;

extern const uint ROUNDING_DIGITS;

extern const uint MAXNUM_CACHE_LEVELS;

extern const uint TILESIZE;

extern const uint TILESIZE_2;
extern const uint TILESIZE_2_BYTES;
extern const uint TILESIZE_X_TILESIZE_2_BYTES;

extern const uint TILEWIDTH_BYTES;
extern const uint TILESIZE_BYTES;
extern const uint LEVEL_S;

extern uint HIGHEST_LEVEL;
extern uint HIGHEST2;

extern uint PRECISION;

extern char OUTPUT_FOLDER[128];

extern bool OSGEO;
extern bool NO_OPTIMIZATION;
extern bool VERBOSE;
extern int BGD;
extern int QUALITY;

extern std::string CRS;

extern std::string HOST;
extern std::string URLSUFFIX;
extern int PORT;

extern std::string LAYER;

extern double BBOX_LEFT;
extern double BBOX_RIGHT;
extern double BBOX_TOP;
extern double BBOX_BOTTOM;

extern double RESOLUTION;
extern double RES2;
