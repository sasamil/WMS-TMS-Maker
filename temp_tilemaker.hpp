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

#ifndef RECURSIVE_TEMPLATEMPG_TMSMAKER_HPP_INCLUDED
#define RECURSIVE_TEMPLATEMPG_TMSMAKER_HPP_INCLUDED

#include <cstring>
#include <fstream>


extern "C" {
    #include <jpeglib.h>
    #include <jerror.h>
}

#include "glconstants.hpp"
#include "tilemaker.hpp"

#define TMS_UNUSED(x) (void)x;

const uint ROUNDING_DIGITS = 2;

const uint MAXNUM_CACHE_LEVELS = 15;

const uint TILESIZE = 512;

const uint TILESIZE_2 = TILESIZE / 2;
const uint TILESIZE_2_BYTES = TILESIZE_2 * 3;
const uint TILESIZE_X_TILESIZE_2_BYTES = TILESIZE * TILESIZE_2_BYTES;

const uint TILEWIDTH_BYTES = TILESIZE * 3;
const uint TILESIZE_BYTES = TILESIZE * TILEWIDTH_BYTES;

fs::path TMS_DIR(fs::current_path());
fs::path RES_DIR(fs::current_path());

#ifdef WIN32
uchar* tilebytes[MAXNUM_CACHE_LEVELS];
#else
uchar tilebytes[MAXNUM_CACHE_LEVELS][TILESIZE_BYTES];
#endif


template <int DIM>
class TmsWriter
{
public:
    static const uchar* outflex(const uint x, const uint y)
    {
        if(DIM > (HIGHEST_LEVEL - LEVEL_S))
            return TmsWriter<DIM-1>::outflex(x, y);

	    if(VERBOSE && DIM == HIGHEST_LEVEL - LEVEL_S - 1)
        {
            std::cout << (1==x && 1==y ? "25%..." : 0==x && 0==y ? "50%..." : 1==x && 0==y ? "75%..." : "");
            std::cout.flush();
        }

#ifndef NDEBUG
        std::cout << "TmsWriter<"<< DIM << ">::outflex(" << x << "," <<  y << ")  (" << (long)tilebytes[DIM] << ")" << std::endl;
#endif
        uint DIM_1 = DIM - 1;
        uint x2 = x << 1;
        uint y2 = y << 1;

        uint row, col;
        uint uired, uigreen, uiblue;
        uchar *iterrow, *iter, *iter2, *iter22;

        if(NULL == TmsWriter<DIM-1>::outflex(x2, y2+1))
        {
            iterrow = tilebytes[DIM];
            for(row=0; row<TILESIZE_2; row++, iterrow += TILEWIDTH_BYTES)
              memset(iterrow, BGD, TILESIZE_2_BYTES);
        }
        else
        {
            iterrow  = tilebytes[DIM], iter = iterrow;
            iter2 = tilebytes[DIM_1], iter22=iter2+TILEWIDTH_BYTES;

            for(row=0; row<TILESIZE_2; row++,
                iterrow  += TILEWIDTH_BYTES, iter=iterrow,
                iter2 = iter22, iter22 += TILEWIDTH_BYTES)
            {
                for(col=0; col<TILESIZE_2; col++)
                {
                    uiblue  = *iter2++;
                    uigreen = *iter2++;
                    uired   = *iter2++;

                    uiblue  += *iter2++;
                    uigreen += *iter2++;
                    uired   += *iter2++;

                    uiblue  += *iter22++;
                    uigreen += *iter22++;
                    uired   += *iter22++;

                    uiblue  += *iter22++;
                    uigreen += *iter22++;
                    uired   += *iter22++;

                    *iter++ = .25*uiblue;
                    *iter++ = .25*uigreen;
                    *iter++ = .25*uired;
                }
            }
        }

        if(NULL == TmsWriter<DIM-1>::outflex(x2+1, y2+1))
        {
            iterrow = tilebytes[DIM] + TILESIZE_2_BYTES;
            for(row=0; row<TILESIZE_2; row++, iterrow += TILEWIDTH_BYTES)
                memset(iterrow, BGD, TILESIZE_2_BYTES);
        }
        else
        {
            iterrow  = tilebytes[DIM] + TILESIZE_2_BYTES, iter = iterrow;
            iter2 = tilebytes[DIM_1], iter22=iter2+TILEWIDTH_BYTES;

            for(row=0; row<TILESIZE_2; row++,
                iterrow  += TILEWIDTH_BYTES, iter=iterrow,
                iter2 = iter22, iter22 += TILEWIDTH_BYTES)
            {
                for(col=0; col<TILESIZE_2; col++)
                {
                    uiblue  = *iter2++;
                    uigreen = *iter2++;
                    uired   = *iter2++;

                    uiblue  += *iter2++;
                    uigreen += *iter2++;
                    uired   += *iter2++;

                    uiblue  += *iter22++;
                    uigreen += *iter22++;
                    uired   += *iter22++;

                    uiblue  += *iter22++;
                    uigreen += *iter22++;
                    uired   += *iter22++;

                    *iter++ = .25*uiblue;
                    *iter++ = .25*uigreen;
                    *iter++ = .25*uired;
                }
            }
        }

        if(NULL == TmsWriter<DIM-1>::outflex(x2, y2))
            return NULL;
        else
        {
            iterrow  = tilebytes[DIM] + TILESIZE_X_TILESIZE_2_BYTES, iter = iterrow;
            iter2 = tilebytes[DIM_1], iter22=iter2+TILEWIDTH_BYTES;

            for(row=0; row<TILESIZE_2; row++,
                iterrow  += TILEWIDTH_BYTES, iter=iterrow,
                iter2 = iter22, iter22 += TILEWIDTH_BYTES)
            {
                for(col=0; col<TILESIZE_2; col++)
                {
                    uiblue  = *iter2++;
                    uigreen = *iter2++;
                    uired   = *iter2++;

                    uiblue  += *iter2++;
                    uigreen += *iter2++;
                    uired   += *iter2++;

                    uiblue  += *iter22++;
                    uigreen += *iter22++;
                    uired   += *iter22++;

                    uiblue  += *iter22++;
                    uigreen += *iter22++;
                    uired   += *iter22++;

                    *iter++ = .25*uiblue;
                    *iter++ = .25*uigreen;
                    *iter++ = .25*uired;
                }
            }
        }

        if(NULL == TmsWriter<DIM-1>::outflex(x2+1, y2))
        {
            iterrow = tilebytes[DIM] + TILESIZE_X_TILESIZE_2_BYTES + TILESIZE_2_BYTES;
            for(row=0; row<TILESIZE_2; row++, iterrow += TILEWIDTH_BYTES)
                memset(iterrow, BGD, TILESIZE_2_BYTES);
        }
        else
        {
            iterrow  = tilebytes[DIM] + TILESIZE_X_TILESIZE_2_BYTES + TILESIZE_2_BYTES, iter = iterrow;
            iter2 = tilebytes[DIM_1], iter22=iter2+TILEWIDTH_BYTES;

            for(row=0; row<TILESIZE_2; row++,
                iterrow  += TILEWIDTH_BYTES, iter=iterrow,
                iter2 = iter22, iter22 += TILEWIDTH_BYTES)
            {
                for(col=0; col<TILESIZE_2; col++)
                {
                    uiblue  = *iter2++;
                    uigreen = *iter2++;
                    uired   = *iter2++;

                    uiblue  += *iter2++;
                    uigreen += *iter2++;
                    uired   += *iter2++;

                    uiblue  += *iter22++;
                    uigreen += *iter22++;
                    uired   += *iter22++;

                    uiblue  += *iter22++;
                    uigreen += *iter22++;
                    uired   += *iter22++;

                    *iter++ = .25*uiblue;
                    *iter++ = .25*uigreen;
                    *iter++ = .25*uired;
                }
            }
        }

        char filename[256];
        sprintf(filename, "%s/%d/%d/%d.jpg", TMS_DIR.string().c_str(), HIGHEST_LEVEL-(DIM+LEVEL_S), x, /*rows_by_levels[HIGHEST_LEVEL-(DIM+LEVEL_S)]-*/y);
        write_jpeg_file(filename, tilebytes[DIM]);

        return tilebytes[DIM];
    }
};


template <>
class TmsWriter<0>
{
  public:
    static const uchar* outflex(const uint x, const uint y)
    {
        TMS_UNUSED(x)
        TMS_UNUSED(y)

#ifndef NDEBUG
        std::cout << "TmsWriter<0>::outflex(" << x << "," <<  y << ")  (" << (long)tilebytes[0] << ")" << std::endl;
#endif

        char filename[256];
        sprintf(filename, "%s/%d/%d/%d.jpg", TMS_DIR.string().c_str(), HIGHEST_LEVEL-LEVEL_S, x, /*rows_by_levels[HIGHEST_LEVEL-LEVEL_S]-*/y);
        FILE* infile = fopen( filename, "rb" );

        if(NULL == infile)
            return NULL;


		struct jpeg_decompress_struct cinfo;
		struct jpeg_error_mgr jerr;
		JSAMPROW row_pointer[1];

		unsigned long location = 0;

		cinfo.err = jpeg_std_error( &jerr );
		jpeg_create_decompress( &cinfo );
		jpeg_stdio_src( &cinfo, infile );
		jpeg_read_header( &cinfo, TRUE );

		jpeg_start_decompress( &cinfo );

		row_pointer[0] = (unsigned char*)malloc( cinfo.output_width*cinfo.num_components );
		while( cinfo.output_scanline < cinfo.image_height )
		{
			jpeg_read_scanlines( &cinfo, row_pointer, 1 );
			for(uint ii=0; ii<cinfo.image_width*cinfo.num_components; ii++)
				tilebytes[0][location++] = row_pointer[0][ii];
		}

		jpeg_finish_decompress( &cinfo );
		jpeg_destroy_decompress( &cinfo );
		free( row_pointer[0] );

		fclose( infile );

        return tilebytes[0];
    }
};

//void (*inflex_p)(const uint, const uint, uchar const * const) = NULL;
const uchar* (*outflex_p)(const uint, const uint) = NULL;


#endif // RECURSIVE_TEMPLATEMPG_TMSMAKER_HPP_INCLUDED
