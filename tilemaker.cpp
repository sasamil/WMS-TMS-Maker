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

#include <cstdlib>
#include <iomanip>

#include "temp_tilemaker.hpp"

union ENDIANESS
{
    unsigned short x;
    unsigned char y;
} endian= {(unsigned short)1};

static const unsigned char bigendian = endian.y;

#define IS_LITTLE_ENDIAN !bigendian

uint _id;

uint _hindex;
uint _vindex;

double _hpos0;
double _hpos2;

double _vpos0;
double _vpos2;

uint NUMROWS = 1;
uint NUMCOLS = 1;

char OUTPUT_FOLDER[128] = "";

int CURRENT_ZOOM = 0;

bool OSGEO = true;

bool NO_OPTIMIZATION = false;
bool VERBOSE = false;
int BGD = 0;
int QUALITY = 90;

std::string CRS = "EPSG:3857";

std::string HOST = "";
std::string URLSUFFIX = "";
int PORT = 80;

std::string LAYER = "";

double BBOX_LEFT = 0.;
double BBOX_RIGHT = 0.;
double BBOX_TOP = 0.;
double BBOX_BOTTOM = 0.;

double BBOX_CENTER_X = 0.;
double BBOX_CENTER_Y = 0.;

uint RIGHT_RESIDUAL = 0;
uint TOP_RESIDUAL = 0;
uint DOWN_JPEG_BYTES = 0;
uint UP_JPEG_BYTES = 0;
uint RIGHT_RESIDUAL_BYTES = 0;

double RESOLUTION = 1.0;
double TILELENGTH = TILESIZE * RESOLUTION;
double RES2 = 0.0;

const uint LEVEL_S = 0;
uint HIGHEST_LEVEL = 0;
uint HIGHEST2 = 0;

uint PRECISION = 8;

bool exceptionAtVeryLastTile = false;

//=================================================================
int count[8];
FILE* outfile[8];

uint jpegbuffer_size;
unsigned char* raw_image;
unsigned char dest_extended[TILESIZE_BYTES ];
unsigned char jpegbuffer[TILESIZE_BYTES ];

static void init_source (j_decompress_ptr cinfo) {}

static boolean fill_input_buffer (j_decompress_ptr cinfo)
{
    ERREXIT(cinfo, JERR_INPUT_EMPTY);
    return TRUE;
}

static void skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
    struct jpeg_source_mgr* src = (struct jpeg_source_mgr*) cinfo->src;

    if (num_bytes > 0) {
        src->next_input_byte += (size_t) num_bytes;
        src->bytes_in_buffer -= (size_t) num_bytes;
    }
}

static void term_source (j_decompress_ptr cinfo) {}

static void jpeg_mem_src (j_decompress_ptr cinfo, void* buffer, long nbytes)
{
    struct jpeg_source_mgr* src;

    if (cinfo->src == NULL) {
        cinfo->src = (struct jpeg_source_mgr *)
            (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(struct jpeg_source_mgr));
    }

    src = (struct jpeg_source_mgr*) cinfo->src;
    src->init_source = init_source;
    src->fill_input_buffer = fill_input_buffer;
    src->skip_input_data = skip_input_data;
    src->resync_to_restart = jpeg_resync_to_restart; /* use default method */
    src->term_source = term_source;
    src->bytes_in_buffer = nbytes;
    src->next_input_byte = (JOCTET*)buffer;
}
int jpg_compressed_bytes_2_uncompressed_bytes()
{
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	JSAMPROW row_pointer[1];
	cinfo.err = jpeg_std_error( &jerr );
	jpeg_create_decompress( &cinfo );
	jpeg_mem_src(&cinfo, jpegbuffer, jpegbuffer_size);
	jpeg_read_header( &cinfo, TRUE );

	jpeg_start_decompress( &cinfo );

	uint width_in_bytes = cinfo.output_width*cinfo.num_components;
	raw_image = (unsigned char*)malloc( cinfo.output_height*width_in_bytes );
	row_pointer[0] = (unsigned char *)malloc( width_in_bytes );
	unsigned char* dest = raw_image;
	for(; cinfo.output_scanline < cinfo.image_height; dest += width_in_bytes )
	{
            jpeg_read_scanlines( &cinfo, row_pointer, 1 );
            memcpy(dest, row_pointer[0], width_in_bytes);
	}

	jpeg_finish_decompress( &cinfo );
	jpeg_destroy_decompress( &cinfo );

	free( row_pointer[0] );

	return 1;
}

//-------------------------------------------------------------------
int write_jpeg_file(const char *filename, uchar* src )
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPROW row_pointer[1];
	FILE *outfile = fopen( filename, "wb" );

	if ( !outfile )
	{
		printf("Error opening output jpeg file %s\n!", filename );
		return -1;
	}
	cinfo.err = jpeg_std_error( &jerr );
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, outfile);
	cinfo.image_width = TILESIZE;
	cinfo.image_height = TILESIZE;
	cinfo.input_components = 3; //bytes_per_pixel;
	cinfo.in_color_space = JCS_RGB;
	jpeg_set_defaults( &cinfo );
	cinfo.num_components = 3;
	cinfo.dct_method = JDCT_FLOAT;
	jpeg_set_quality(&cinfo, QUALITY, TRUE);
	jpeg_start_compress( &cinfo, TRUE );
	for(row_pointer[0] = src; cinfo.next_scanline < cinfo.image_height; row_pointer[0] += TILEWIDTH_BYTES)
		jpeg_write_scanlines( &cinfo, row_pointer, 1 );

	jpeg_finish_compress( &cinfo );
	jpeg_destroy_compress( &cinfo );
	fclose( outfile );

	return 1;
}

int UpNullDownJpeg2Jpeg(const char* outputfilename)
{
    memset(dest_extended, BGD, UP_JPEG_BYTES);
    memcpy(dest_extended+UP_JPEG_BYTES, raw_image, DOWN_JPEG_BYTES);
    free(raw_image);
    if(write_jpeg_file(outputfilename, dest_extended) != 1) return -1;

    return 1;
}

int LeftJpegRightNull2Jpeg(const char* outputfilename)
{
    memset(dest_extended, BGD, TILESIZE_BYTES);
    uchar* src2 = raw_image;
    uchar* dest2 = dest_extended;
    for(uint ii=0; ii<TILESIZE; ++ii, dest2+=TILEWIDTH_BYTES, src2+=RIGHT_RESIDUAL_BYTES)
        memcpy(dest2, src2, RIGHT_RESIDUAL_BYTES);

    free(raw_image);
    if(write_jpeg_file(outputfilename, dest_extended) != 1) return -1;

    return 1;
}

int LeftDownJpegRightUpNull2Jpeg(const char* outputfilename)
{
    memset(dest_extended, BGD, TILESIZE_BYTES);
    uchar* src2 = raw_image;
    uchar* dest2 = dest_extended + UP_JPEG_BYTES;
    for(uint ii=TILESIZE-TOP_RESIDUAL; ii<TILESIZE; ++ii, dest2+=TILEWIDTH_BYTES, src2+=RIGHT_RESIDUAL_BYTES)
        memcpy(dest2, src2, RIGHT_RESIDUAL_BYTES);

    free(raw_image);

    if(write_jpeg_file(outputfilename, dest_extended) != 1) return -1;

    return 1;
}

std::string globalstr = "";
void OnBeginTest( const happyhttp::Response* r, void* userdata )
{
    int procesid = *((int*)userdata);

	std::string str(r->getheader("content-type"));
	std::string::size_type idx1 = str.rfind("/xml");
	std::string::size_type idx2 = str.rfind("/txt");
	std::string::size_type idx3 = str.rfind("/html");
	if(idx1==str.length()-4 || idx2==str.length()-4 || idx3==str.length()-5)
		globalstr = "\nThe response from server:\n\n";

    else if(!(200==r->getstatus() && 1389 == procesid && 0 == strcmp(r->getheader("content-type"), "image/jpeg")))
        throw("Possible Problem at OnBeginTest");
}

void OnDataTest( const happyhttp::Response* r, void* userdata, const unsigned char* data, int n )
{
    int procesid = *((int*)userdata);
	if(globalstr != "")
		for(int ii=0; ii<n; ii++)
			globalstr += (data[ii]);

    else if(!(200==r->getstatus() && 1389 == procesid && data != NULL && n>0))
        throw("Possible Problem at OnDataTest");
}

void OnCompleteTest( const happyhttp::Response* r, void* userdata )
{
    int procesid = *((int*)userdata);
	if(globalstr != "")
		throw(globalstr.c_str());

	else if(!(200==r->getstatus() && 1389 == procesid ))
        throw("Possible Problem at OnCompleteTest");
}


void OnBegin( const happyhttp::Response* r, void* userdata )
{
    int procesid = *((int*)userdata);
    int idirnumber  = procesid / NUMROWS;
    int ifilenumber = procesid % NUMROWS;
    char filename[128];

    sprintf(filename, "%s/%d/%d/%d.jpg", TMS_DIR.string().c_str(), CURRENT_ZOOM, idirnumber, ifilenumber);
    outfile[procesid%8] = fopen( filename, "wb" );

    count[procesid%8] = 0;

    if(VERBOSE)
        std::cout << "BEGIN Response " << " (" << r->getstatus() << " " << r->getreason() << ")" << "  " << r->getheader("content-type") << std::endl;

    if( 0 != strcmp(r->getheader("content-type"), "image/jpeg") )
        throw("\nThe response is not regular jpeg image?!\nPlease, check the connection, url, given options...\n");
}

void OnData( const happyhttp::Response* r, void* userdata, const unsigned char* data, int n )
{
    int procesid = *((int*)userdata);
    fwrite( data,1,n, outfile[procesid%8] );
    count[procesid%8] += n;
}

void OnComplete( const happyhttp::Response* r, void* userdata )
{
    int procesid = *((int*)userdata);
    fclose( outfile[procesid%8] );

    if(VERBOSE)
        std::cout << "COMPLETE Response " << " (" << count[procesid%8] << " bytes)   " << 100.0*(1.0+procesid)/NUMCOLS/NUMROWS << "%" << std::endl;
}


void OnBeginSpecial( const happyhttp::Response* r, void* userdata )
{
    TMS_UNUSED(userdata)

    jpegbuffer_size = 0;

    if(VERBOSE)
        std::cout << "BEGIN SPECIAL Response " << " (" << r->getstatus() << " " << r->getreason() << ")" << "  " << r->getheader("content-type") << std::endl;

    if( 0 != strcmp(r->getheader("content-type"), "image/jpeg") )
        throw("\nThe response is not regular jpeg image?!\nPlease, check the connection, url, given options...\n");
}

void OnDataSpecial( const happyhttp::Response* r, void* userdata, const unsigned char* data, int n )
{
    TMS_UNUSED(userdata)

    memcpy(jpegbuffer+jpegbuffer_size, data, n);
    jpegbuffer_size += n;
}

void OnCompleteDown( const happyhttp::Response* r, void* userdata )
{
    int procesid = *((int*)userdata);
    int idirnumber  = procesid / NUMROWS;
    int ifilenumber = procesid % NUMROWS;
    char filename[128];

    sprintf(filename, "%s/%d/%d/%d.jpg", TMS_DIR.string().c_str(), CURRENT_ZOOM, idirnumber, ifilenumber);

    jpg_compressed_bytes_2_uncompressed_bytes();
    UpNullDownJpeg2Jpeg(filename);

    if(VERBOSE)
        std::cout << "COMPLETE DOWN Response " << " (" << jpegbuffer_size << " bytes)   " << 100.0*(1.0+procesid)/NUMCOLS/NUMROWS << "%" << std::endl;
}

void OnCompleteLeft( const happyhttp::Response* r, void* userdata )
{
    int procesid = *((int*)userdata);
    int idirnumber  = procesid / NUMROWS;
    int ifilenumber = procesid % NUMROWS;
    char filename[128];

    sprintf(filename, "%s/%d/%d/%d.jpg", TMS_DIR.string().c_str(), CURRENT_ZOOM, idirnumber, ifilenumber);

    jpg_compressed_bytes_2_uncompressed_bytes();
    LeftJpegRightNull2Jpeg(filename);

    if(VERBOSE)
        std::cout << "COMPLETE LEFT Response " << " (" << jpegbuffer_size << " bytes)   " << 100.0*(1.0+procesid)/NUMCOLS/NUMROWS << "%" << std::endl;
}

void OnCompleteLeftDown( const happyhttp::Response* r, void* userdata )
{
    int procesid = *((int*)userdata);
    int idirnumber  = procesid / NUMROWS;
    int ifilenumber = procesid % NUMROWS;
    char filename[128];

    sprintf(filename, "%s/%d/%d/%d.jpg", TMS_DIR.string().c_str(), CURRENT_ZOOM, idirnumber, ifilenumber);

    jpg_compressed_bytes_2_uncompressed_bytes();
    LeftDownJpegRightUpNull2Jpeg(filename);

    if(VERBOSE)
        std::cout << "COMPLETE LEFT-DOWN Response " << " (" << jpegbuffer_size << " bytes)   " << 100.0*(1.0+procesid)/NUMCOLS/NUMROWS << "%" << std::endl;
}


double roundoff(double num, uint precision)
{
    char str[16];
    switch (precision)
    {
    case 0:
        sprintf(str, "%.0f", num);
        break;
    case 1:
        sprintf(str, "%.1f", num);
        break;
    case 2:
        sprintf(str, "%.2f", num);
        break;
    case 3:
        sprintf(str, "%.3f", num);
        break;
    case 4:
        sprintf(str, "%.4f", num);
        break;
    case 5:
        sprintf(str, "%.5f", num);
        break;
    case 6:
        sprintf(str, "%.6f", num);
        break;
    case 7:
        sprintf(str, "%.7f", num);
        break;
    case 8:
        sprintf(str, "%.8f", num);
        break;

    default:
        sprintf(str, "%.4f", num);
    }

    return atof(str);
}

void updateGlobalConstants()
{
    BBOX_CENTER_X = (BBOX_LEFT+BBOX_RIGHT)/2;
    BBOX_CENTER_Y = (BBOX_TOP+BBOX_BOTTOM)/2;

    if(RES2 != 0.0)
    {
        double resl = RESOLUTION;
        HIGHEST2 = 0;
        while(resl < RES2)
        {
            resl *= 2.0;
            ++HIGHEST2;
        }
    }

    const double scenewidth = BBOX_RIGHT - BBOX_LEFT;
    const double sceneheight = BBOX_TOP - BBOX_BOTTOM;
    const double scenesyze = scenewidth > sceneheight ? scenewidth : sceneheight;
    double cis = TILESIZE * RESOLUTION;
    HIGHEST_LEVEL = 0;
    while(cis < scenesyze)
    {
        cis *= 2.0;
        ++HIGHEST_LEVEL;
    }

    double dres = RESOLUTION;
    PRECISION = 0;
    while(dres - (int)dres > 1e-8)
    {
        dres *= 10.0;
        ++PRECISION;
    }

    switch (HIGHEST_LEVEL - LEVEL_S)
    {
    case 0:
        outflex_p = TmsWriter<0>::outflex;
        break;
    case 1:
        outflex_p = TmsWriter<1>::outflex;
        break;
    case 2:
        outflex_p = TmsWriter<2>::outflex;
        break;
    case 3:
        outflex_p = TmsWriter<3>::outflex;
        break;
    case 4:
        outflex_p = TmsWriter<4>::outflex;
        break;
    case 5:
        outflex_p = TmsWriter<5>::outflex;
        break;
    case 6:
        outflex_p = TmsWriter<6>::outflex;
        break;
    case 7:
        outflex_p = TmsWriter<7>::outflex;
        break;
    case 8:
        outflex_p = TmsWriter<8>::outflex;
        break;
    case 9:
        outflex_p = TmsWriter<9>::outflex;
        break;

    default:
        outflex_p = NULL;
        break;
    }
}

void createDirs(const char* chdirtms)
{
    char subfolder[8], subsubfolder[8];

    if(chdirtms)
    {
        TMS_DIR = fs::path(chdirtms);
        if(!fs::exists(TMS_DIR))
            fs::create_directory(TMS_DIR);
    }

    TMS_DIR /= OUTPUT_FOLDER;
    fs::create_directory(TMS_DIR);

    fs::path EXCEPTION_DIR = TMS_DIR / "wmsxmls";
    fs::create_directory(EXCEPTION_DIR);

    if(OSGEO)
    {
        TMS_DIR /= "1.0.0";
        fs::create_directory(TMS_DIR);
        TMS_DIR /= "tms_static_cache";
        fs::create_directory(TMS_DIR);
    }

    NUMCOLS = static_cast<uint>((BBOX_RIGHT - BBOX_LEFT) / TILESIZE / RESOLUTION);
    if( BBOX_RIGHT - BBOX_LEFT - TILESIZE*RESOLUTION*NUMCOLS > RESOLUTION || 0==NUMCOLS)
        ++NUMCOLS;


    uint numcols = NUMCOLS;
    for(int jj=HIGHEST_LEVEL; jj>=0; jj--)
    {
        sprintf(subfolder, "%d", jj);
        fs::create_directory(TMS_DIR / subfolder);

        for(uint ii=0; ii<numcols; ii++)
        {
            sprintf(subfolder, "%d", jj);
            sprintf(subsubfolder, "%d", ii);
            fs::create_directory(TMS_DIR / subfolder / subsubfolder);
        }

        ++numcols /= 2;
    }
}

void makeCacheLevel(int level)
{
	if(VERBOSE)
        std::cout << std::endl << std::endl << "======== Making cache level " << level << " ========" << std::endl << std::endl;

    CURRENT_ZOOM = level;

	double tempres = RESOLUTION * (1 << (HIGHEST_LEVEL-level));

    TILELENGTH = TILESIZE * tempres;

    const double scenewidth = BBOX_RIGHT - BBOX_LEFT;
	const double sceneheight = BBOX_TOP - BBOX_BOTTOM;

	NUMCOLS = static_cast<uint>( scenewidth / TILELENGTH );
    RIGHT_RESIDUAL = static_cast<int>(0.5 + (scenewidth - TILELENGTH*NUMCOLS)/tempres);
    RIGHT_RESIDUAL_BYTES = RIGHT_RESIDUAL * 3;
    if( RIGHT_RESIDUAL > 0 || 0==NUMCOLS)
        ++NUMCOLS;

    NUMROWS = static_cast<uint>( sceneheight / TILELENGTH );
    TOP_RESIDUAL = static_cast<int>(0.5 + (sceneheight - TILELENGTH*NUMROWS)/tempres);
    if( TOP_RESIDUAL > 0 || 0==NUMROWS)
        ++NUMROWS;

	std::cout << std::fixed << std::setprecision( static_cast<int>(std::log10(NUMROWS*NUMCOLS)) - 1 ); // 4 verbose

    DOWN_JPEG_BYTES = TOP_RESIDUAL * TILESIZE * 3;
    UP_JPEG_BYTES = TILESIZE_BYTES - DOWN_JPEG_BYTES;

    _hpos0 = BBOX_LEFT,   _hpos2 = _hpos0 + TILELENGTH, _hindex = 0;
    _vpos0 = BBOX_BOTTOM, _vpos2 = _vpos0 + TILELENGTH, _vindex = 0;
    _id = 0;

    while(1+_id < NUMROWS*NUMCOLS)
		makeTmsBase();

	if(exceptionAtVeryLastTile)
		makeTmsBase();
}

//-------------------------------------------------------------------
void makeHttpTest()
{
    std::stringstream strstream;
    strstream << std::fixed << std::setprecision(PRECISION);
    try
    {
        happyhttp::Connection conn = happyhttp::Connection( HOST.c_str(), PORT );

        strstream << URLSUFFIX
                  << "?SERVICE=WMS&VERSION=1.1.1&REQUEST=GetMap"
                  << "&LAYERS=" << LAYER
                  << "&FORMAT=image/jpeg"
                  << "&SRS=" << CRS
                  << "&BBOX="
                  << BBOX_LEFT << "," << BBOX_BOTTOM << ","
                  << BBOX_LEFT+(TILESIZE*RESOLUTION) << ","
                  << BBOX_BOTTOM+(TILESIZE*RESOLUTION)
				  << "&WIDTH=" << TILESIZE
				  << "&HEIGHT=" << TILESIZE;

        int id = 1389;
        conn.setcallbacks( OnBeginTest, OnDataTest, OnCompleteTest, &id );
        conn.request( "GET", strstream.str().c_str(), 0, 0, 0 );

        while( conn.outstanding() )
            conn.pump();
    }

    catch( happyhttp::Wobbly& e )
    {
        std::cout << "Initial HTTP Test failed." << std::endl << std::endl
                  << "The request:" << std::endl << std::endl
				  << HOST << ":" << PORT << strstream.str() << std::endl
                  << e.what() << std::endl
                  << "Program terminated!" << std::endl;

#ifdef WIN32
getchar();
#endif

        _exit(0);
    }

    catch(const char* message)
    {
        std::cout << "Initial HTTP Test failed." << std::endl << std::endl
                  << "The request:" << std::endl << std::endl
				  << HOST << ":" << PORT << strstream.str() << std::endl
                  << message << std::endl
                  << "Program terminated!" << std::endl;

#ifdef WIN32
getchar();
#endif

        _exit(0);
    }

    catch (...)
    {
        std::cout << "Initial HTTP Test failed due to the unknown and unexpected problem." << std::endl
                  << "The request:  " << HOST << ":" << PORT << strstream.str() << std::endl
                  << "Program terminated!" << std::endl;

#ifdef WIN32
getchar();
#endif

        _exit(0);
    }
}

void makeTmsBase()
{
    std::stringstream strstream;
    strstream << std::fixed << std::setprecision(PRECISION);

    try
	{
        happyhttp::Connection conn = happyhttp::Connection( HOST.c_str(), PORT );

        if(_hpos2 < BBOX_RIGHT)
        {
            for(; _vpos2 < BBOX_TOP; _vpos0 = _vpos2, _vpos2 += TILELENGTH, ++_vindex, ++_id)
            {
                if(VERBOSE)
                    std::cout << std::endl << "wms-request " << std::hex << std::showbase << _id+1 << std::dec << "   " <<  _hindex << " " << _vindex << std::endl;

                strstream.str(std::string());
                strstream << URLSUFFIX
                          << "?SERVICE=WMS&VERSION=1.1.1&REQUEST=GetMap&FORMAT=image/jpeg"
                          << "&LAYERS=" << LAYER
                          << "&SRS=" << CRS
                          << "&BBOX=" << _hpos0 << "," << _vpos0 << "," << _hpos2 << "," << _vpos2
                          << "&WIDTH=" << TILESIZE
                          << "&HEIGHT=" << TILESIZE;

                conn.setcallbacks( OnBegin, OnData, OnComplete, &_id );
                conn.request( "GET", strstream.str().c_str(), 0, 0, 0 );

                while( conn.outstanding() )
                    conn.pump();
            }

            if(TOP_RESIDUAL > 0)
			{
				if(VERBOSE)
					std::cout << std::endl << "wms-request " << std::hex << std::showbase << _id+1 << std::dec << "   " <<  _hindex << " " << _vindex << std::endl;

				strstream.str(std::string());
				strstream << URLSUFFIX
						  << "?SERVICE=WMS&VERSION=1.1.1&REQUEST=GetMap&FORMAT=image/jpeg"
						  << "&LAYERS=" << LAYER
						  << "&SRS=" << CRS
						  << "&BBOX=" << _hpos0 << "," << _vpos0 << "," << _hpos2 << "," << BBOX_TOP
						  << "&WIDTH=" << TILESIZE
						  << "&HEIGHT=" << TOP_RESIDUAL;

				conn.setcallbacks( OnBeginSpecial, OnDataSpecial, OnCompleteDown, &_id );
				conn.request( "GET", strstream.str().c_str(), 0, 0, 0 );

				while( conn.outstanding() )
					conn.pump();

				++_id;
			}
        }

        else
        {
            for(; _vpos2 < BBOX_TOP; _vpos0 = _vpos2, _vpos2 += TILELENGTH, ++_vindex, ++_id)
            {
                if(VERBOSE)
                    std::cout << std::endl << "wms-request " << std::hex << std::showbase << _id+1 << std::dec << "   " <<  _hindex << " " << _vindex << std::endl;

                strstream.str(std::string());
                strstream << URLSUFFIX
                          << "?SERVICE=WMS&VERSION=1.1.1&REQUEST=GetMap&FORMAT=image/jpeg"
                          << "&LAYERS=" << LAYER
                          << "&SRS=" << CRS
                          << "&BBOX=" << _hpos0 << "," << _vpos0 << "," << BBOX_RIGHT << "," << _vpos2
                          << "&WIDTH=" << RIGHT_RESIDUAL
                          << "&HEIGHT=" << TILESIZE;

                conn.setcallbacks( OnBeginSpecial, OnDataSpecial, OnCompleteLeft, /*0*/ &_id );
                conn.request( "GET", strstream.str().c_str(), 0, 0, 0 );

                while( conn.outstanding() )
                    conn.pump();
            }

            if(TOP_RESIDUAL > 0)
			{
				if(VERBOSE)
					std::cout << std::endl << "wms-request " << std::hex << std::showbase << _id+1 << std::dec << "   " <<  _hindex << " " << _vindex << std::endl;

				strstream.str(std::string());
				strstream << URLSUFFIX
						  << "?SERVICE=WMS&VERSION=1.1.1&REQUEST=GetMap&FORMAT=image/jpeg"
						  << "&LAYERS=" << LAYER
						  << "&SRS=" << CRS
						  << "&BBOX=" << _hpos0 << "," << _vpos0 << "," << BBOX_RIGHT << "," << BBOX_TOP
						  << "&WIDTH=" << RIGHT_RESIDUAL
						  << "&HEIGHT=" << TOP_RESIDUAL;

				conn.setcallbacks( OnBeginSpecial, OnDataSpecial, OnCompleteLeftDown, &_id );
				conn.request( "GET", strstream.str().c_str(), 0, 0, 0 );

				while( conn.outstanding() )
					conn.pump();
			}

			return;
        }


        for (_hpos0 = _hpos2, _hpos2 += TILELENGTH, ++_hindex;
                _hpos2 < BBOX_RIGHT;
                _hpos0 = _hpos2, _hpos2 += TILELENGTH, ++_hindex)
        {
            for(_vpos0 = BBOX_BOTTOM, _vpos2 = _vpos0 + TILELENGTH, _vindex = 0;
                    _vpos2 < BBOX_TOP;
                    _vpos0 = _vpos2, _vpos2 += TILELENGTH, ++_vindex, ++_id)
            {
                if(VERBOSE)
                    std::cout << std::endl << "wms-request " << std::hex << std::showbase << _id+1 << std::dec << "   " <<  _hindex << " " << _vindex << std::endl;

                strstream.str(std::string());
                strstream << URLSUFFIX
                          << "?SERVICE=WMS&VERSION=1.1.1&REQUEST=GetMap&FORMAT=image/jpeg"
                          << "&LAYERS=" << LAYER
                          << "&SRS=" << CRS
                          << "&BBOX=" << _hpos0 << "," << _vpos0 << "," << _hpos2 << "," << _vpos2
                          << "&WIDTH=" << TILESIZE
                          << "&HEIGHT=" << TILESIZE;

                conn.setcallbacks( OnBegin, OnData, OnComplete, /*0*/ &_id );
                conn.request( "GET", strstream.str().c_str(), 0, 0, 0 );

                while( conn.outstanding() )
                    conn.pump();
            }

            if(TOP_RESIDUAL > 0)
			{
				if(VERBOSE)
					std::cout << std::endl << "wms-request " << std::hex << std::showbase << _id+1 << std::dec << "   " <<  _hindex << " " << _vindex << std::endl;

				strstream.str(std::string());
				strstream << URLSUFFIX
						  << "?SERVICE=WMS&VERSION=1.1.1&REQUEST=GetMap&FORMAT=image/jpeg"
						  << "&LAYERS=" << LAYER
						  << "&SRS=" << CRS
						  << "&BBOX=" << _hpos0 << "," << _vpos0 << "," << _hpos2 << "," << BBOX_TOP
						  << "&WIDTH=" << TILESIZE
						  << "&HEIGHT=" << TOP_RESIDUAL;

				conn.setcallbacks( OnBeginSpecial, OnDataSpecial, OnCompleteDown, &_id );
				conn.request( "GET", strstream.str().c_str(), 0, 0, 0 );

				while( conn.outstanding() )
					conn.pump();

				++_id;
			}
        }

        if(RIGHT_RESIDUAL > 0)
		{
			for(_vpos0 = BBOX_BOTTOM, _vpos2 = _vpos0 + TILELENGTH, _vindex = 0;
					_vpos2 < BBOX_TOP;
					_vpos0 = _vpos2, _vpos2 += TILELENGTH, ++_vindex, ++_id)
			{
				if(VERBOSE)
					std::cout << std::endl << "wms-request " << std::hex << std::showbase << _id+1 << std::dec << "   " <<  _hindex << " " << _vindex << std::endl;

				strstream.str(std::string());
				strstream << URLSUFFIX
						  << "?SERVICE=WMS&VERSION=1.1.1&REQUEST=GetMap&FORMAT=image/jpeg"
						  << "&LAYERS=" << LAYER
						  << "&SRS=" << CRS
						  << "&BBOX=" << _hpos0 << "," << _vpos0 << "," << BBOX_RIGHT << "," << _vpos2
						  << "&WIDTH=" << RIGHT_RESIDUAL
						  << "&HEIGHT=" << TILESIZE;

				conn.setcallbacks( OnBeginSpecial, OnDataSpecial, OnCompleteLeft, /*0*/ &_id );
				conn.request( "GET", strstream.str().c_str(), 0, 0, 0 );

				while( conn.outstanding() )
					conn.pump();
			}

			if(TOP_RESIDUAL > 0)
			{
				if(VERBOSE)
					std::cout << std::endl << "wms-request " << std::hex << std::showbase << _id+1 << std::dec << "   " <<  _hindex << " " << _vindex << std::endl;

				strstream.str(std::string());
				strstream << URLSUFFIX
						  << "?SERVICE=WMS&VERSION=1.1.1&REQUEST=GetMap&FORMAT=image/jpeg"
						  << "&LAYERS=" << LAYER
						  << "&SRS=" << CRS
						  << "&BBOX=" << _hpos0 << "," << _vpos0 << "," << BBOX_RIGHT << "," << BBOX_TOP
						  << "&WIDTH=" << RIGHT_RESIDUAL
						  << "&HEIGHT=" << TOP_RESIDUAL;

				conn.setcallbacks( OnBeginSpecial, OnDataSpecial, OnCompleteLeftDown, &_id );
				conn.request( "GET", strstream.str().c_str(), 0, 0, 0 );

				while( conn.outstanding() )
					conn.pump();
			}
		}
    }

    catch( happyhttp::Wobbly& e )
    {
        std::string str(e.what());
		if( str.find("Connection closed")==std::string::npos && str.find("connection abort")==std::string::npos )
        {
            std::cout << "Exception during the execution:" << std::endl
                      << "The request:  " << HOST << ":" << PORT << strstream.str() << std::endl
                      << e.what() << std::endl
                      << "Program terminated!" << std::endl;

            _exit(0);
        }

		exceptionAtVeryLastTile = (NUMROWS*NUMCOLS == 1+_id);
    }

    catch(const char* message)
    {
        std::cout << "Exception during the execution:" << std::endl
                  << "The request:  " << HOST << ":" << PORT << strstream.str() << std::endl
                  << message << std::endl
                  << "Program terminated!" << std::endl;

        _exit(0);
    }

    catch (...)
    {
        std::cout << "Exception during the execution due to the unknown and unexpected problem." << std::endl
                  << "The request:  " << HOST << ":" << PORT << strstream.str() << std::endl
                  << "Program terminated!" << std::endl;

        _exit(0);
    }
}

void makeTmsComplete()
{
    updateGlobalConstants();

    createDirs(TMS_DIR.string().c_str());

    if(!NO_OPTIMIZATION)
    {
        makeCacheLevel(HIGHEST_LEVEL);

#ifdef WIN32
        for(uint ii=0; ii<=HIGHEST_LEVEL; ii++)
            tilebytes[ii] = new uchar[TILESIZE_BYTES];
#endif

        if(VERBOSE)
        {
            std::cout << std::endl << "Outflex processing: 0%...";
            std::cout.flush();
        }
        outflex_p(0, 0);
        if(VERBOSE)
            std::cout << "100% - OK!" << std::endl;

#ifdef WIN32
        for(uint ii=0; ii<=HIGHEST_LEVEL; ii++)
            delete[] tilebytes[ii];
#endif
    }

    else
        for(int jj=HIGHEST_LEVEL; jj>=0; jj--)
            makeCacheLevel(jj);
}

void makeTmsSampleOL2_strict()
{
    std::ofstream out("sample-osgeo_tms_ol2.html");

    out << std::fixed << std::setprecision(PRECISION);
    out << "<!DOCTYPE html>" << std::endl
        << "<html>" << std::endl
        << "<head>" << std::endl
        << "    <title>TileMaker Sample</title>" << std::endl
        << "    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/>" << std::endl
        << "    <script type=\"text/javascript\" language=\"JavaScript1.2\" src=\"http://www.openlayers.org/api/OpenLayers.js\"></script>" << std::endl
        << "</head>\n" << std::endl

        << "  <body style=\"background-color: #222222; font-family: 'Arial'; font-size: 80%; font-weight: normal;\">" << std::endl
        << "  	<h1 style=\"font-size: 150%; color: #3333ee; text-align: center; padding-top: 20px; padding-bottom: 3px; border-bottom: 1px solid white;\">OSGeo TMS Example - Open Layers 2</h1>" << std::endl
        << "  	<div id=\"olmap\" style=\"position: absolute; left: 200px; right: 200px; top: 150px; bottom: 100px; border:3px solid white; background-color: " << (0==BGD ? "black" : "white") << ";\"></div>" << std::endl
        << "  	<div style=\"position: absolute; left: 20px; right: 20px; bottom: 35px; text-align: center; color: #bb0000;\">Generated by TileMaker</div>" << std::endl << std::endl

        << "    <script type=\"text/javascript\">" << std::endl
        << "      var maxext = new OpenLayers.Bounds(" << BBOX_LEFT << ", " << BBOX_BOTTOM << ", " << BBOX_RIGHT << ", " << BBOX_TOP << ");" << std::endl
        << "      var map = new OpenLayers.Map('olmap', {units: 'm', maxExtent: maxext});" << std::endl
        << "      var tms = new OpenLayers.Layer.TMS('tmslayer', '" << OUTPUT_FOLDER << "/'," << std::endl
        << "    					{layername: 'tms_static_cache', type:'jpg', maxExtent: maxext, tileSize: new OpenLayers.Size(" << TILESIZE << "," << TILESIZE << "), resolutions: [";

    double tempres = RESOLUTION * std::pow(2.0, HIGHEST_LEVEL);
    for(uint ii=0; ii<HIGHEST_LEVEL; ii++)
    {
        out << tempres << ", ";
        tempres /= 2.;
    }

    out << tempres << "]});" << std::endl
        << "      map.addLayer(tms);" << std::endl
        << "      map.addControl(new OpenLayers.Control.KeyboardDefaults());" << std::endl
        << "      map.setCenter(new OpenLayers.LonLat(" << BBOX_CENTER_X << ", " << BBOX_CENTER_Y << "), " << HIGHEST_LEVEL-1 << ");" << std::endl
        << "    </script>\n" << std::endl

        << "  </body>" << std::endl
        << "</html>" << std::endl;

    out.close();
}

void makeTmsSampleOL2_amended()
{
    std::ofstream out("sample-tms_ol2.html");

    out << std::fixed << std::setprecision(PRECISION);
    out << "<!DOCTYPE html>" << std::endl
        << "<html>" << std::endl
        << "<head>" << std::endl
        << "    <title>TileMaker Sample</title>" << std::endl
        << "    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/>" << std::endl
        << "    <!--script type=\"text/javascript\" language=\"JavaScript1.2\" src=\"http://www.openlayers.org/api/OpenLayers.js\"></script-->" << std::endl
        << "    <script type=\"text/javascript\" language=\"JavaScript1.2\" src=\"http://www.geomreze.rgz.gov.rs/openlayers/OpenLayers.js\"></script>" << std::endl
        << "</head>\n" << std::endl

        << "  <body style=\"background-color: #222222; font-family: 'Arial'; font-size: 80%; font-weight: normal;\">" << std::endl
        << "  	<h1 style=\"font-size: 150%; color: #3333ee; text-align: center; padding-top: 20px; padding-bottom: 3px; border-bottom: 1px solid white;\">TMS Example - Open Layers 2</h1>" << std::endl
        << "  	<div id=\"olmap\" style=\"position: absolute; left: 200px; right: 200px; top: 150px; bottom: 100px; border:3px solid white; background-color: " << (0==BGD ? "black" : "white") << ";\"></div>" << std::endl
        << "  	<div style=\"position: absolute; left: 20px; right: 20px; bottom: 35px; text-align: center; color: #bb0000;\">Generated by TileMaker</div>" << std::endl << std::endl

        << "    <script type=\"text/javascript\">" << std::endl
        << "      // Hack to support local tiles is needed because string \"1.0.0\" is hardcoded in OL source for url" << std::endl
        << "      // New, derived TMS class" << std::endl
        << "      var TMS2 = OpenLayers.Class(OpenLayers.Layer.TMS, {" << std::endl
        << "          getURL: function (bounds) {" << std::endl
        << "          var res = this.map.getResolution();" << std::endl
        << "          var x = Math.round ((bounds.left - this.maxExtent.left) / (res * this.tileSize.w));" << std::endl
        << "          var y = Math.round ((bounds.bottom - this.maxExtent.bottom) / (res * this.tileSize.h));" << std::endl
        << "          var z = this.map.getZoom();" << std::endl << std::endl

        << "          var path = z + \"/\" + x + \"/\" + y + \".\" + this.type;" << std::endl
        << "          var url = this.url;" << std::endl
        << "          if (url instanceof Array) {" << std::endl
        << "              url = this.selectUrl(path, url);" << std::endl
        << "          }" << std::endl << std::endl

        << "          return url + path;" << std::endl
        << "          }," << std::endl
        << "          CLASS_NAME: \"TMS2\"" << std::endl
        << "      });" << std::endl << std::endl

        << "      var maxext = new OpenLayers.Bounds(" << BBOX_LEFT << ", " << BBOX_BOTTOM << ", " << BBOX_RIGHT << ", " << BBOX_TOP << ");" << std::endl
        << "      var map = new OpenLayers.Map('olmap', {units: 'm', maxExtent: maxext});" << std::endl
        << "      var tms = new TMS2('tmslayer', '" << OUTPUT_FOLDER << "/', {layername: 'tms_static_cache', type:'jpg'," << std::endl
        << "                                      maxExtent: maxext, tileSize: new OpenLayers.Size(" << TILESIZE << "," << TILESIZE << ")," << std::endl
        << "                                      resolutions: [";

    double tempres = RESOLUTION * std::pow(2.0, HIGHEST_LEVEL);
    for(uint ii=0; ii<HIGHEST_LEVEL; ii++)
    {
        out << tempres << ", ";
        tempres /= 2.;
    }

    out << tempres << "]});" << std::endl
        << "      map.addLayer(tms);" << std::endl
        << "      map.addControl(new OpenLayers.Control.KeyboardDefaults());" << std::endl
        << "      map.setCenter(new OpenLayers.LonLat(" << BBOX_CENTER_X << ", " << BBOX_CENTER_Y << "), " << HIGHEST_LEVEL-1 << ");" << std::endl
        << "    </script>\n" << std::endl

        << "  </body>" << std::endl
        << "</html>" << std::endl;

    out.close();
}

void makeTmsSampleOL3()
{
    std::ofstream out("sample-osgeo_tms_ol3.html");

    out << std::fixed << std::setprecision(PRECISION);
    out << "<!DOCTYPE html>" << std::endl
        << "<html>" << std::endl
        << "<head>" << std::endl
        << "    <title>TileMaker Sample</title>" << std::endl
        << "    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/>" << std::endl
        << "    <link rel=\"stylesheet\" href=\"http://openlayers.org/en/v3.10.1/css/ol.css\" type=\"text/css\">" << std::endl
        << "    <script type=\"text/javascript\" language=\"JavaScript1.2\" src=\"http://openlayers.org/en/v3.10.1/build/ol.js\"></script>" << std::endl
        << "</head>\n" << std::endl

        << "  <body style=\"background-color: #222222; font-family: 'Arial'; font-size: 80%; font-weight: normal;\">" << std::endl
        << "  	<h1 style=\"font-size: 150%; color: #3333ee; text-align: center; padding-top: 20px; padding-bottom: 3px; border-bottom: 1px solid white;\">OSGeo TMS Example - Open Layers 3</h1>" << std::endl
        << "  	<div id=\"olmap\" style=\"position: absolute; left: 200px; right: 200px; top: 150px; bottom: 100px; border:3px solid white; background-color: " << (0==BGD ? "black" : "white") << ";\"></div>" << std::endl
        << "  	<div style=\"position: absolute; left: 20px; right: 20px; bottom: 35px; text-align: center; color: #bb0000;\">Generated by TileMaker</div>" << std::endl << std::endl

        << "    <script type=\"text/javascript\">" << std::endl
        << "    	var extent = [" << BBOX_LEFT << ", " << BBOX_BOTTOM << ", " << BBOX_RIGHT << ", " << BBOX_TOP << "], resolutions = [";

    double tempres = RESOLUTION * std::pow(2.0, HIGHEST_LEVEL);
    for(uint ii=0; ii<HIGHEST_LEVEL; ii++)
    {
        out << tempres << ", ";
        tempres /= 2.;
    }

    out	<< tempres << "];" << std::endl
        << "    	var projection = ol.proj.get('" << CRS << "');" << std::endl
        << "    	var map = new ol.Map({" << std::endl
        << "		  target: 'olmap'," << std::endl
        << "		  layers: [" << std::endl
        << "			new ol.layer.Tile({" << std::endl
        << "			  preload: Infinity," << std::endl
        << "			  source: new ol.source.TileImage({" << std::endl
        << "				tileUrlFunction: function(coordinate) {" << std::endl
        << "				  return 'tms/1.0.0/tms_static_cache/'+coordinate[0]+'/'+ coordinate[1] +'/'+ coordinate[2] +'.jpg';" << std::endl
        << "				}," << std::endl
        << "				projection: projection," << std::endl
        << "				tileGrid: new ol.tilegrid.TileGrid({" << std::endl
        << "				  extent: extent," << std::endl
        << "				  tileSize: [" << TILESIZE << ", " << TILESIZE << "]," << std::endl
        << "				  origin: ol.extent.getBottomLeft(extent)," << std::endl
        << "				  resolutions: resolutions" << std::endl
        << "				})" << std::endl
        << "			  })" << std::endl
        <<			   "})" << std::endl
        << "		  ]," << std::endl
        << "		  view: new ol.View({" << std::endl
        << "			extent: extent," << std::endl
        << "			resolutions: resolutions," << std::endl
        << "			center: ol.extent.getCenter(extent)," << std::endl
        << "			zoom: " << HIGHEST_LEVEL-1 << std::endl
        << "		  })" << std::endl
        << "	    });" << std::endl
        << "    </script>\n" << std::endl

        << "  </body>" << std::endl
        << "</html>" << std::endl;

    out.close();
}

void makeWmsSampleOL2()
{
    std::ofstream out("sample-wms_ol2.html");

    out << std::fixed << std::setprecision(PRECISION);
    out << "<!DOCTYPE html>" << std::endl
        << "<html>" << std::endl
        << "<head>" << std::endl
        << "    <title>TileMaker Sample</title>" << std::endl
        << "    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/>" << std::endl
        << "    <script type=\"text/javascript\" language=\"JavaScript1.2\" src=\"http://www.openlayers.org/api/OpenLayers.js\"></script>" << std::endl
        << "</head>\n" << std::endl

        << "  <body style=\"background-color: #222222; font-family: 'Arial'; font-size: 80%; font-weight: normal;\">" << std::endl
        << "  	<h1 style=\"font-size: 150%; color: #3333ee; text-align: center; padding-top: 20px; padding-bottom: 3px; border-bottom: 1px solid white;\">WMS Example - Open Layers 2</h1>" << std::endl
        << "  	<div id=\"olmap\" style=\"position: absolute; left: 200px; right: 200px; top: 150px; bottom: 100px; border:3px solid white; background-color: " << (0==BGD ? "black" : "white") << ";\"></div>" << std::endl
        << "  	<div style=\"position: absolute; left: 20px; right: 20px; bottom: 35px; text-align: center; color: #bb0000;\">Generated by TileMaker</div>" << std::endl << std::endl

        << "    <script type=\"text/javascript\">" << std::endl
        << "    var maxext = new OpenLayers.Bounds(" << BBOX_LEFT << ", " << BBOX_BOTTOM << ", " << BBOX_RIGHT << ", " << BBOX_TOP << ");" << std::endl
        << "    var map = new OpenLayers.Map('olmap', {units: 'm', maxExtent: maxext," << std::endl
        << "                               projection: new OpenLayers.Projection(\"" << CRS << "\")," << std::endl
        << "                               resolutions: [";

    double tempres = RESOLUTION * std::pow(2.0, HIGHEST_LEVEL);
    for(uint ii=0; ii<HIGHEST_LEVEL; ii++)
    {
        out << tempres << ", ";
        tempres /= 2.;
    }

    out << tempres << "]});" << std::endl << std::endl // write up the lowest resolution and we can go on further...
        << "	var wms = new OpenLayers.Layer.WMS('WMS layer', 'http://localhost:81/tilemakerwms/wms.php'," << std::endl
        << "									{layers: 'tmscache'}, {singleTile: true, isBaseLayer: true});" << std::endl << std::endl

        << "	map.addLayer(wms);" << std::endl
        << "	map.addControl(new OpenLayers.Control.KeyboardDefaults());" << std::endl
        << "    map.setCenter(new OpenLayers.LonLat(" << BBOX_CENTER_X << ", " << BBOX_CENTER_Y << "), " << HIGHEST_LEVEL-1 << ");" << std::endl
        << "    </script>\n" << std::endl

        << "  </body>" << std::endl
        << "</html>" << std::endl;

    out.close();
}

void makeWmsSampleOL3()
{
    std::ofstream out("sample-wms_ol3.html");

    out << std::fixed << std::setprecision(PRECISION);
    out << "<!DOCTYPE html>" << std::endl
        << "<html>" << std::endl
        << "<head>" << std::endl
        << "    <title>TileMaker Sample</title>" << std::endl
        << "    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/>" << std::endl
        << "    <link rel=\"stylesheet\" href=\"http://openlayers.org/en/v3.10.0/css/ol.css\" type=\"text/css\">" << std::endl
        << "    <script type=\"text/javascript\" language=\"JavaScript1.2\" src=\"http://openlayers.org/en/v3.10.0/build/ol.js\"></script>" << std::endl
        << "</head>\n" << std::endl

        << "  <body style=\"background-color: #222222; font-family: 'Arial'; font-size: 80%; font-weight: normal;\">" << std::endl
        << "  	<h1 style=\"font-size: 150%; color: #3333ee; text-align: center; padding-top: 20px; padding-bottom: 3px; border-bottom: 1px solid white;\">WMS Example - Open Layers 3</h1>" << std::endl
        << "  	<div id=\"olmap\" style=\"position: absolute; left: 200px; right: 200px; top: 150px; bottom: 100px; border:3px solid white; background-color: " << (0==BGD ? "black" : "white") << ";\"></div>" << std::endl
        << "  	<div style=\"position: absolute; left: 20px; right: 20px; bottom: 35px; text-align: center; color: #bb0000;\">Generated by TileMaker</div>" << std::endl << std::endl

        << "    <script type=\"text/javascript\">" << std::endl
        << "        var prj = new ol.proj.Projection({" << std::endl
        << "            code: '" << CRS << "'," << std::endl
        << "            extent: [" << BBOX_LEFT << ", " << BBOX_BOTTOM << ", " << BBOX_RIGHT << ", " << BBOX_TOP << "], // The extent is used to determine zoom level 0" << std::endl
        << "            units: 'm'" << std::endl
        << "        })" << std::endl
        << "        ol.proj.addProjection(prj);" << std::endl
        << std::endl
        << "        var layers = [ new ol.layer.Image({/*extent: [453200.05, 4958799.95, 455600.05, 4961599.95],*/" << std::endl
        << "                                           source: new ol.source.ImageWMS({" << std::endl
        << "                                               url: 'http://localhost:81/tilemakerwms/wms.php'," << std::endl
        << "                                               params: {'LAYERS': 'tmscache', 'FORMAT': 'image/jpeg'}" << std::endl
        << "                                           })" << std::endl
        << "                                          })" << std::endl
        << "                     ];" << std::endl
        << "        var map = new ol.Map({" << std::endl
        << "            layers: layers," << std::endl
        << "            target: 'olmap'," << std::endl
        << "            view: new ol.View({" << std::endl
        << "                center: [" << BBOX_CENTER_X << ", " << BBOX_CENTER_Y << "]," << std::endl
        << "                resolutions: [";

    double tempres = RESOLUTION * std::pow(2.0, HIGHEST_LEVEL);
    for(uint ii=0; ii<HIGHEST_LEVEL; ii++)
    {
        out << tempres << ", ";
        tempres /= 2.;
    }

    out << tempres << "]," << std::endl
        << "                projection: prj," << std::endl
        << "                zoom: " << HIGHEST_LEVEL << std::endl
        << "            })" << std::endl
        << "        });" << std::endl
        << "    </script>" << std::endl
        << std::endl
        << "  </body>" << std::endl
        << "</html>" << std::endl;

    out.close();
}

void makeWmsSampleArcGisJSApi()
{
    std::ofstream out("sample-wms_arcgis.html");

    const char* arcgisCRS = CRS.c_str()+5;

    out << std::fixed << std::setprecision(PRECISION);
    out << "<!DOCTYPE html>" << std::endl
        << "<html>" << std::endl
        << "<head>" << std::endl
        << "    <title>TileMaker Sample</title>" << std::endl
        << "    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\"/>" << std::endl
        << "    <link rel=\"stylesheet\" href=\"https://js.arcgis.com/3.17/esri/css/esri.css\" type=\"text/css\">" << std::endl
        << "    <script type=\"text/javascript\" language=\"JavaScript1.2\" src=\"https://js.arcgis.com/3.17/\"></script>" << std::endl
        << "</head>\n" << std::endl

        << "  <body style=\"background-color: #222222; font-family: 'Arial'; font-size: 80%; font-weight: normal;\">" << std::endl
        << "  	<h1 style=\"font-size: 150%; color: #3333ee; text-align: center; padding-top: 20px; padding-bottom: 3px; border-bottom: 1px solid white;\">WMS Example - ArcGIS API for JavaScript</h1>" << std::endl
        << "  	<div id=\"map\" style=\"position: absolute; left: 200px; right: 200px; top: 150px; bottom: 100px; border:3px solid white; background-color: " << (0==BGD ? "black" : "white") << ";\"></div>" << std::endl
        << "  	<div style=\"position: absolute; left: 20px; right: 20px; bottom: 35px; text-align: center; color: #bb0000;\">Generated by TileMaker</div>" << std::endl << std::endl

        << "    <script type=\"text/javascript\">" << std::endl
        << "        var map;" << std::endl
        << "        require(['esri/map', 'esri/layers/WMSLayer', 'esri/layers/WMSLayerInfo', 'esri/geometry/Extent'," << std::endl
        << "                 'dojo/_base/array', 'dojo/dom', 'dojo/dom-construct', 'dojo/parser']," << std::endl
        << "                function(Map, WMSLayer, WMSLayerInfo, Extent, array, dom, domConst, parser) {" << std::endl
        << std::endl
        << "          parser.parse();" << std::endl
        << std::endl
        << "          map = new Map('map', {spatialReferences: [" << arcgisCRS << "], center: [" << BBOX_CENTER_X << ", " << BBOX_CENTER_Y << "]});" << std::endl
        << std::endl
        << "          var layer1 = new WMSLayerInfo({name: 'tmscache', title: 'WMS layer'});" << std::endl
        << "          var resourceInfo1 = {format: 'jpg'," << std::endl
        << "                               extent: new Extent(" << BBOX_LEFT << ", "<< BBOX_BOTTOM << ", " << BBOX_RIGHT << ", " << BBOX_TOP << ", {wkid: " << arcgisCRS << "})," << std::endl
        << "                               layerInfos: [layer1]," << std::endl
        << "                               spatialReferences: [" << arcgisCRS << "]," << std::endl
        << "                               version: '1.1.1'};" << std::endl
        << std::endl
        << "          var wmsLayer = new WMSLayer('http://localhost:81/tilemakerwms/wms.php', {resourceInfo: resourceInfo1, visibleLayers: ['tmscache']});" << std::endl
        << std::endl
        << "          map.addLayers([wmsLayer]);" << std::endl
        << "          for(i=0; i<" << HIGHEST_LEVEL-2 << "; i++) map.setLevel(0);" << std::endl
        << "        });" << std::endl
        << "    </script>" << std::endl
        << std::endl
        << "  </body>" << std::endl
        << "</html>" << std::endl;

    out.close();
}

void makeWmsPhp()
{
    char phppath[128];
    sprintf(phppath, "%s/wms.php", OUTPUT_FOLDER);
    std::ofstream out(phppath);

    out << std::fixed << std::setprecision(PRECISION);

    out << "<?php" << std::endl
        << std::endl
        << "function LoadJPEG($imgname) {" << std::endl
        << "    // Attempt to open" << std::endl
        << "    $im = @imagecreatefromjpeg($imgname);" << std::endl
        << "    // See if it failed" << std::endl
        << "    if (!$im) {" << std::endl
        << "        // Create a blank image" << std::endl
        << "        global $tileHeight, $tileWidth;" << std::endl
        << "        $im = imagecreatetruecolor($tileWidth, $tileHeight);" << std::endl
        << "        $bgc = imagecolorallocate($im, " << (0==BGD ? "0, 0, 0" : "255, 255, 255") << ");" << std::endl
        << "        imagefilledrectangle($im, 0, 0, $tileWidth, $tileHeight, $bgc);" << std::endl
        << "    }" << std::endl
        << "    return $im;" << std::endl
        << "}" << std::endl
        << std::endl
        << "function jointiles() {" << std::endl
        << "    global $zum, $tileHeight, $tileWidth, $foldermin, $foldermax, $tilemin, $tilemax, $pixelLeft, $pixelRight, $pixelUp, $pixelDown, $width, $height;" << std::endl
        << "    $path = './1.0.0/tms_static_cache/' . $zum . '/';" << std::endl
        << std::endl
        << "    $iOut = imagecreatetruecolor(($foldermax - $foldermin + 1) * $tileWidth, ($tilemax - $tilemin + 1) * $tileHeight);" << std::endl
        << "    for ($i = $foldermin; $i <= $foldermax ; $i++) {" << std::endl
        << "        for ($j = $tilemin; $j <= $tilemax; $j++) {" << std::endl
        << "            $imgtemp = LoadJPEG($path . $i. '/' .  $j . '.jpg');" << std::endl
        << "            imagecopy($iOut, $imgtemp, ($i-$foldermin) * $tileWidth, ($tilemax-$j) * $tileHeight, 0, 0, $tileWidth, $tileHeight);" << std::endl
        << "            imagedestroy($imgtemp);" << std::endl
        << "        }" << std::endl
        << "    }" << std::endl
        << "    $iOutFinal = imagecreatetruecolor($width, $height);" << std::endl
        << "    imagecopyresized($iOutFinal, $iOut, 0, 0, $pixelLeft, $pixelUp, $width, $height, imagesx($iOut) - $pixelLeft - $pixelRight, imagesy($iOut) - $pixelUp - $pixelDown);" << std::endl
        << "    return $iOutFinal;" << std::endl
        << "}" << std::endl
        << std::endl
        << std::endl
        << "function getZoom($res) {" << std::endl
        << "    global $resolutions;" << std::endl
        << std::endl
        << "    $zoom = count($resolutions)-1;" << std::endl
        << "    $query = $res < $resolutions[$zoom];" << std::endl
        << "    if($query)" << std::endl
        << "        return $zoom;" << std::endl
        << std::endl
        << "    for ($i = $zoom ; $i > 0; $i--) {" << std::endl
        << "        $query = (($res >= $resolutions[$i]) && ($res <= $resolutions[$i-1]));" << std::endl
        << "        if ($query) {" << std::endl
        << "            $d1 = $res - $resolutions[$i];" << std::endl
        << "            $d2 = $resolutions[$i-1] - $res;" << std::endl
        << "            $zoom = ($d1 < $d2) ? $i : ($i-1);" << std::endl
        << "            return $zoom;" << std::endl
        << "        }" << std::endl
        << "    }" << std::endl
        << std::endl
        << "    return 0;" << std::endl
        << "}" << std::endl
        << std::endl
        << "function setParameters($box) {" << std::endl
        << "    global $xmin, $xmax, $ymin, $ymax, $xresolution, $yresolution, $width, $height, $dlx, $dly, $urx, $ury, $srs;" << std::endl
        << "    $coord = explode(',', $box);" << std::endl
        << "    if ( count($coord)!=4 || !is_numeric($width) || !is_numeric($height) || " << std::endl
        << "    					!is_numeric($coord[0]) || !is_numeric($coord[0]) || !is_numeric($coord[1]) || !is_numeric($coord[1]) ) " << std::endl
        << "    {" << std::endl
        << "    	header('Content-type: text/xml');" << std::endl
        << "    	$filestring = file_get_contents('./wmsxmls/errorBadDims.xml');" << std::endl
        << "    	echo $filestring;" << std::endl
        << "    	exit;" << std::endl
        << "    }" << std::endl
        << std::endl
        << "    $xmin = doubleval($coord[0]);" << std::endl
        << "    $ymin = doubleval($coord[1]);" << std::endl
        << "    $xmax = doubleval($coord[2]);" << std::endl
        << "    $ymax = doubleval($coord[3]);" << std::endl
        << std::endl
        << "    if ( $width < 0 || $height < 0 || $xmin >= $xmax || $ymin >= $ymax ) {" << std::endl
        << "    	header('Content-type: text/xml');" << std::endl
        << "    	$filestring = file_get_contents('./wmsxmls/errorBadDims.xml');" << std::endl
        << "    	echo $filestring;" << std::endl
        << "    	exit;" << std::endl
        << "    }" << std::endl
        << std::endl
        << "    if ( $width < 16 || $height < 16 || $width > 2400 || $height > 2400  ) {" << std::endl
        << "    	header('Content-type: text/xml');" << std::endl
        << "    	$filestring = file_get_contents('./wmsxmls/errorBigSmall.xml');" << std::endl
        << "    	echo $filestring;" << std::endl
        << "    	exit;" << std::endl
        << "    }" << std::endl
        << std::endl
        << "    $xdiff = $xmax - $xmin;" << std::endl
        << "    $ydiff = $ymax - $ymin;" << std::endl
        << "    $xresolution = $xdiff / $width;" << std::endl
        << "    $yresolution = $ydiff / $height;" << std::endl
        << "}" << std::endl
        << std::endl
        << std::endl
        << "$dlx = " << BBOX_LEFT << ";" << std::endl
        << "$dly = " << BBOX_BOTTOM << ";" << std::endl
        << "$urx = " << BBOX_RIGHT << ";" << std::endl
        << "$ury = " << BBOX_TOP << ";" << std::endl
        << std::endl
        << "$tileWidth = " << TILESIZE << ";" << std::endl
        << "$tileHeight = " << TILESIZE << ";" << std::endl
        << std::endl
        << "$resolutions = array(";

    double tempres = RESOLUTION * std::pow(2.0, HIGHEST_LEVEL);
    for(int ii=HIGHEST_LEVEL-1; ii>=0; ii--)
    {
        out << tempres << ", ";
        tempres /= 2.;
    }

    out << tempres << ");" << std::endl
        << std::endl
        << "$requesttype = strtoupper($_GET['REQUEST']);" << std::endl
        << "if('GETCAPABILITIES' == $requesttype) {" << std::endl
        << "	$service = strtoupper($_GET['SERVICE']);" << std::endl
        << "	if('WMS' != $service) {" << std::endl
        << "		header('Content-type: text/xml');" << std::endl
        << "		$filestring = file_get_contents('./wmsxmls/errorService.xml');" << std::endl
        << "		echo $filestring;" << std::endl
        << "		exit;" << std::endl
        << "		}" << std::endl
        << std::endl
        << "	header('Content-type: text/xml');" << std::endl
        << "	$filestring = file_get_contents('./wmsxmls/capabilities.xml');" << std::endl
        << "	echo $filestring;" << std::endl
        << "	exit;" << std::endl
        << "}" << std::endl
        << std::endl
        << "else if('GETFEATUREINFO' == $requesttype) {" << std::endl
        << "	header('Content-type: text/xml');" << std::endl
        << "	$filestring = file_get_contents('./wmsxmls/errorQueryable.xml');" << std::endl
        << "	echo $filestring;" << std::endl
        << "	exit;" << std::endl
        << "}" << std::endl
        << std::endl
        << "else if ('GETMAP' == $requesttype) {" << std::endl
        << "    $srs = $_GET['SRS'];" << std::endl
        << "    if(''==$srs) $srs = $_GET['CRS']; //OL3" << std::endl
        << std::endl
        << "    if('" << CRS << "' != $srs && 'EPSG:0' != $srs) {" << std::endl
        << "    	header('Content-type: text/xml');" << std::endl
        << "    	$filestring = file_get_contents('./wmsxmls/errorCRS.xml');" << std::endl
        << "    	echo $filestring;" << std::endl
        << "    	exit;" << std::endl
        << "    }" << std::endl
        << std::endl
        << "    $format = $_GET['FORMAT'];" << std::endl
        << "    if('image/jpeg' != $format && 'image/jpg' != $format) {" << std::endl
        << "    	header('Content-type: text/xml');" << std::endl
        << "    	$filestring = file_get_contents('./wmsxmls/errorFormat.xml');" << std::endl
        << "    	echo $filestring;" << std::endl
        << "    	exit;" << std::endl
        << "    }" << std::endl
        << std::endl
        << "    $layer = $_GET['LAYERS'];" << std::endl
        << "    if('tmscache' != $layer) {" << std::endl
        << "    	header('Content-type: text/xml');" << std::endl
        << "    	$filestring = file_get_contents('./wmsxmls/errorLayer.xml');" << std::endl
        << "    	echo $filestring;" << std::endl
        << "    	exit;" << std::endl
        << "    }" << std::endl
        << std::endl
        << "    $bbox = $_GET['BBOX'];" << std::endl
        << "    $width = $_GET['WIDTH'];" << std::endl
        << "    $height = $_GET['HEIGHT'];" << std::endl
        << "    if('' == $width || '' == $height || '' == $bbox) {" << std::endl
        << "    	header('Content-type: text/xml');" << std::endl
        << "    	$filestring = file_get_contents('./wmsxmls/errorNoDims.xml');" << std::endl
        << "    	echo $filestring;" << std::endl
        << "    	exit;" << std::endl
        << "    }" << std::endl
        << std::endl
        << "    $xmin; $xmax; $ymin; $ymax; $xresolution; $yresolution; $resolution;" << std::endl
        << std::endl
        << "    setParameters($bbox);" << std::endl
        << "    $resolution = $xresolution > $yresolution ? $xresolution : $yresolution;" << std::endl
        << std::endl
        << "    $zum = getZoom($resolution);" << std::endl
        << "    $foldermin = floor(($xmin - $dlx) / ($tileWidth * $resolutions[$zum]));" << std::endl
        << "    $foldermax = floor(($xmax - $dlx) / ($tileWidth * $resolutions[$zum]));" << std::endl
        << "    $tilemin = floor(($ymin - $dly) / ($tileHeight * $resolutions[$zum]));" << std::endl
        << "    $tilemax = floor(($ymax - $dly) / ($tileHeight * $resolutions[$zum]));" << std::endl
        << std::endl
        << "    $pixelLeft =  (int) ( ($xmin - $dlx) / $resolutions[$zum] - $foldermin * $tileWidth );" << std::endl
        << "    $pixelRight = (int) ( ($foldermax + 1) * $tileWidth  - ($xmax - $dlx) / $resolutions[$zum] );" << std::endl
        << "    $pixelDown =  (int) ( ($ymin - $dly) / $resolutions[$zum] - $tilemin * $tileHeight );" << std::endl
        << "    $pixelUp =  (int) ( ($tilemax + 1) * $tileHeight - ($ymax - $dly) / $resolutions[$zum] );" << std::endl
        << std::endl
        << "    header('Content-type: image/jpeg');" << std::endl
        << "    $outFinal = jointiles();" << std::endl
        << "    imagejpeg($outFinal);" << std::endl
        << "    imagedestroy($outFinal);" << std::endl
        << "}" << std::endl
        << std::endl
        << "else {" << std::endl
        << "	header('Content-type: text/xml');" << std::endl
        << "	$filestring = file_get_contents('./wmsxmls/errorRequest.xml');" << std::endl
        << "	echo $filestring;" << std::endl
        << "	exit;" << std::endl
        << "}" << std::endl
        << "?>" << std::endl;

    out.close();
}

void makeWmsCapabilites()
{
    char xmlpath[128];
    sprintf(xmlpath, "%s/wmsxmls/capabilities.xml", OUTPUT_FOLDER);
    std::ofstream out(xmlpath);

    out << std::fixed << std::setprecision(PRECISION);

    out << "<?xml version='1.0' encoding=\"UTF-8\"?>" << std::endl
        << "<!DOCTYPE WMT_MS_Capabilities SYSTEM \"http://schemas.opengis.net/wms/1.1.1/capabilities_1_1_1.dtd\">" << std::endl
        << "<WMT_MS_Capabilities version=\"1.1.1\">" << std::endl
        << "  <Service>" << std::endl
        << "    <Name>OGC:WMS</Name>" << std::endl
        << "    <Title>Strict OSGeo TMS tile cache, Generated by TileMaker</Title>" << std::endl
        << "    <OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:type=\"simple\" xlink:href=\"http://localhost:81/tmsmaker/wms.php\"/>" << std::endl
        << "    <MaxWidth>2400</MaxWidth>" << std::endl
        << "    <MaxHeight>2400</MaxHeight>" << std::endl
        << "  </Service>" << std::endl
        << "  <Capability>" << std::endl
        << "    <Request>" << std::endl
        << "      <GetCapabilities>" << std::endl
        << "        <Format>application/vnd.ogc.wms_xml</Format>" << std::endl
        << "        <DCPType>" << std::endl
        << "          <HTTP>" << std::endl
        << "            <Get>" << std::endl
        << "              <OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:type=\"simple\" xlink:href=\"http://localhost:81/tmsmaker/wms.php\"/>" << std::endl
        << "            </Get>" << std::endl
        << "            <!--Post>" << std::endl
        << "              <OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:type=\"simple\" xlink:href=\"http://localhost:81/tmsmaker/wms.php\"/>" << std::endl
        << "            </Post-->" << std::endl
        << "          </HTTP>" << std::endl
        << "        </DCPType>" << std::endl
        << "      </GetCapabilities>" << std::endl
        << "      <GetMap>" << std::endl
        << "        <Format>image/jpeg</Format>" << std::endl
        << "        <DCPType>" << std::endl
        << "          <HTTP>" << std::endl
        << "            <Get>" << std::endl
        << "              <OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:type=\"simple\" xlink:href=\"http://localhost:81/tmsmaker/wms.php\"/>" << std::endl
        << "            </Get>" << std::endl
        << "            <!--Post>" << std::endl
        << "              <OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:type=\"simple\" xlink:href=\"http://localhost:81/tmsmaker/wms.php\"/>" << std::endl
        << "            </Post-->" << std::endl
        << "          </HTTP>" << std::endl
        << "        </DCPType>" << std::endl
        << "      </GetMap>" << std::endl
        << "    </Request>" << std::endl
        << "    <Exception>" << std::endl
        << "      <Format>application/vnd.ogc.se_xml</Format>" << std::endl
        << "    </Exception>" << std::endl
        << "    <Layer queryable=\"0\" opaque=\"1\">" << std::endl
        << "      <Name>tmscache</Name>" << std::endl
        << "      <Title>Base WMS layer</Title>" << std::endl
        << "      <Abstract>Base WMS layer created from mosaic tiff structure</Abstract>" << std::endl
        << "      <CRS>" << CRS << "</CRS>" << std::endl
        << "      <LatLonBoundingBox minx=\"" << 114.0 << "\" miny=\"" << -35.2 << "\" maxx=\"" << 120.0 << "\" maxy=\"" << -19.6 << "\"/>" << std::endl
        << "      <BoundingBox CRS=\"" << CRS << "\" minx=\"" << BBOX_LEFT << "\" miny=\"" << BBOX_BOTTOM << "\" maxx=\"" << BBOX_RIGHT << "\"  maxy=\"" << BBOX_TOP << "\"/>" << std::endl
        << "	  <ScaleHint min=\"" << 100 << "\" max=\"" << 6400 << "\"/>" << std::endl
        << "    </Layer>" << std::endl
        << "  </Capability>" << std::endl
        << "</WMT_MS_Capabilities>" << std::endl;

    out.close();
}

//-------------------------------------------------------------------
void makeExceptionXmls()
{
    char xmlpath[128];

    sprintf(xmlpath, "%s/wmsxmls/errorBadDims.xml", OUTPUT_FOLDER);
    std::ofstream out1(xmlpath);
    out1 << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl
         << "<ServiceExceptionReport version=\"1.3.0\"" << std::endl
         << "xmlns=\"http://www.opengis.net/ogc\"" << std::endl
         << "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl
         << "xsi:schemaLocation=\"http://www.opengis.net/ogc http://schemas.opengis.net/wms/1.3.0/exceptions_1_3_0.xsd\">" << std::endl
         << "  <ServiceException code=\"InvalidDimensionValue\">Request contains an invalid sample dimension value.</ServiceException>" << std::endl
         << "</ServiceExceptionReport>" << std::endl;
    out1.close();

    sprintf(xmlpath, "%s/wmsxmls/errorBigSmall.xml", OUTPUT_FOLDER);
    std::ofstream out2(xmlpath);
    out2 << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl
         << "<ServiceExceptionReport version=\"1.3.0\"" << std::endl
         << "xmlns=\"http://www.opengis.net/ogc\"" << std::endl
         << "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl
         << "xsi:schemaLocation=\"http://www.opengis.net/ogc http://schemas.opengis.net/wms/1.3.0/exceptions_1_3_0.xsd\">" << std::endl
         << "  <ServiceException code=\"InvalidDimensionValue\">Invalid request. Requested raster is too big or too small.</ServiceException>" << std::endl
         << "</ServiceExceptionReport>" << std::endl;
    out2.close();

    sprintf(xmlpath, "%s/wmsxmls/errorCRS.xml", OUTPUT_FOLDER);
    std::ofstream out3(xmlpath);
    out3 << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl
         << "<ServiceExceptionReport version=\"1.3.0\"" << std::endl
         << "xmlns=\"http://www.opengis.net/ogc\"" << std::endl
         << "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl
         << "xsi:schemaLocation=\"http://www.opengis.net/ogc http://schemas.opengis.net/wms/1.3.0/exceptions_1_3_0.xsd\">" << std::endl
         << "  <ServiceException code=\"InvalidDimensionValue\">Request contains a CRS not offered by the server. This WMS-server accepts only " << CRS << " requests.</ServiceException>" << std::endl
         << "</ServiceExceptionReport>" << std::endl;
    out3.close();

    sprintf(xmlpath, "%s/wmsxmls/errorFormat.xml", OUTPUT_FOLDER);
    std::ofstream out4(xmlpath);
    out4 << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl
         << "<ServiceExceptionReport version=\"1.3.0\"" << std::endl
         << "xmlns=\"http://www.opengis.net/ogc\"" << std::endl
         << "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl
         << "xsi:schemaLocation=\"http://www.opengis.net/ogc http://schemas.opengis.net/wms/1.3.0/exceptions_1_3_0.xsd\">" << std::endl
         << "  <ServiceException code=\"InvalidDimensionValue\">Request contains a Format not offered by the server. This WMS-server returns only jpeg raster images.</ServiceException>" << std::endl
         << "</ServiceExceptionReport>" << std::endl;
    out4.close();

    sprintf(xmlpath, "%s/wmsxmls/errorLayer.xml", OUTPUT_FOLDER);
    std::ofstream out5(xmlpath);
    out5 << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl
         << "<ServiceExceptionReport version=\"1.3.0\"" << std::endl
         << "xmlns=\"http://www.opengis.net/ogc\"" << std::endl
         << "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl
         << "xsi:schemaLocation=\"http://www.opengis.net/ogc http://schemas.opengis.net/wms/1.3.0/exceptions_1_3_0.xsd\">" << std::endl
         << "  <ServiceException code=\"InvalidDimensionValue\">GetMap request is for a Layer not offered by the server. So far, this WMS-server accepts only requests about 'tmscache' layer.</ServiceException>" << std::endl
         << "</ServiceExceptionReport>" << std::endl;
    out5.close();

    sprintf(xmlpath, "%s/wmsxmls/errorNoDims.xml", OUTPUT_FOLDER);
    std::ofstream out6(xmlpath);
    out6 << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl
         << "<ServiceExceptionReport version=\"1.3.0\"" << std::endl
         << "xmlns=\"http://www.opengis.net/ogc\"" << std::endl
         << "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl
         << "xsi:schemaLocation=\"http://www.opengis.net/ogc http://schemas.opengis.net/wms/1.3.0/exceptions_1_3_0.xsd\">" << std::endl
         << "  <ServiceException code=\"InvalidDimensionValue\">Request does not include needed dimension parameters.</ServiceException>" << std::endl
         << "</ServiceExceptionReport>" << std::endl;
    out6.close();

    sprintf(xmlpath, "%s/wmsxmls/errorOut.xml", OUTPUT_FOLDER);
    std::ofstream out7(xmlpath);
    out7 << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl
         << "<ServiceExceptionReport version=\"1.3.0\"" << std::endl
         << "xmlns=\"http://www.opengis.net/ogc\"" << std::endl
         << "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl
         << "xsi:schemaLocation=\"http://www.opengis.net/ogc http://schemas.opengis.net/wms/1.3.0/exceptions_1_3_0.xsd\">" << std::endl
         << "  <ServiceException code=\"InvalidDimensionValue\">Requests falls completely out of declared BBOX for this WMS server.</ServiceException>" << std::endl
         << "</ServiceExceptionReport>" << std::endl;
    out7.close();

    sprintf(xmlpath, "%s/wmsxmls/errorQeryable.xml", OUTPUT_FOLDER);
    std::ofstream out8(xmlpath);
    out8 << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl
         << "<ServiceExceptionReport version=\"1.3.0\"" << std::endl
         << "xmlns=\"http://www.opengis.net/ogc\"" << std::endl
         << "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl
         << "xsi:schemaLocation=\"http://www.opengis.net/ogc http://schemas.opengis.net/wms/1.3.0/exceptions_1_3_0.xsd\">" << std::endl
         << "  <ServiceException code=\"InvalidDimensionValue\">GetFeatureInfo request is applied to a layer which does not reply to queries.</ServiceException>" << std::endl
         << "</ServiceExceptionReport>" << std::endl;
    out8.close();

    sprintf(xmlpath, "%s/wmsxmls/errorRequest.xml", OUTPUT_FOLDER);
    std::ofstream out9(xmlpath);
    out9 << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl
         << "<ServiceExceptionReport version=\"1.3.0\"" << std::endl
         << "xmlns=\"http://www.opengis.net/ogc\"" << std::endl
         << "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl
         << "xsi:schemaLocation=\"http://www.opengis.net/ogc http://schemas.opengis.net/wms/1.3.0/exceptions_1_3_0.xsd\">" << std::endl
         << "  <ServiceException code=\"InvalidDimensionValue\">Invalid request. Only GetMap and GetCapabilities requests are allowed for this WMS server.</ServiceException>" << std::endl
         << "</ServiceExceptionReport>" << std::endl;
    out9.close();

    sprintf(xmlpath, "%s/wmsxmls/errorService.xml", OUTPUT_FOLDER);
    std::ofstream out10(xmlpath);
    out10 << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl
          << "<ServiceExceptionReport version=\"1.3.0\"" << std::endl
          << "xmlns=\"http://www.opengis.net/ogc\"" << std::endl
          << "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" << std::endl
          << "xsi:schemaLocation=\"http://www.opengis.net/ogc http://schemas.opengis.net/wms/1.3.0/exceptions_1_3_0.xsd\">" << std::endl
          << "  <ServiceException code=\"InvalidDimensionValue\">Missing or wrong SERVICE parameter. WMS is the only one allowed value.</ServiceException>" << std::endl
          << "</ServiceExceptionReport>" << std::endl;
    out10.close();
}

