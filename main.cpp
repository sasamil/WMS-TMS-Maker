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

#include <ctime>
#include "tilemaker.hpp"
#include "happyhttp.h"

#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

namespace po = boost::program_options;

#ifdef WIN32
#include <winsock2.h>
#endif



//===================================================================
int main(int argc, char* argv[])
{
    	std::ios::sync_with_stdio(false);

    	std::cout /*<< "Hello from tilemaker_wms"*/ << std::endl;

#ifdef WIN32
    	WSAData wsaData;
    	int code = WSAStartup(MAKEWORD(1, 1), &wsaData);
    	if( code != 0 )
    	{
		printf("shite. %d\n",code);
		return 0;
    	}
#endif
	try
	{
		int quality;
		double resolution;
		std::vector<double> bbox;
        	std::string url, layer, crs, background, sbox;

        	po::options_description desc("Allowed options");
        	desc.add_options()
            	("help", "produce help message")
            	("url,u", po::value<std::string>(&url), "url-address of wms service")
            	("layer,l", po::value<std::string>(&layer), "layer served by wms service")
            	("bbox,x", po::value<std::string>(&sbox)->multitoken(), "area extent (comma separated string)")
            	("res,r", po::value<double>(&resolution), "maximal (best) resolution")
            	("crs,c", po::value<std::string>(&crs)->default_value("EPSG:3857"), "spatial reference system code (optional)") // default_value !
            	("quality,q", po::value<int>(&quality)->default_value(90), "jpeg compression quality (optional)") // default_value !
            	("background,b", po::value<std::string>(&background)->default_value("white"), "background color [black | white] (optional)")
            	("verbose,v", "follow the progress on the screen (optional)")
            	("no-opt,n", "no optimization (recommended for the maps with generalized features at lower scales) (optional)")
			;

       		po::variables_map vm;
        	po::store(po::parse_command_line(argc, argv, desc), vm);
        	po::notify(vm);

		if (vm.count("help"))
		{
            		std::cout << desc << std::endl;
#ifdef WIN32
getchar();
#endif
            		return 0;
        	}

        	if (vm.count("url"))
		{
			std::string::size_type idx;

			idx = url.find("http://");
			if (0 == idx)
				url.replace(0, 7, "");

			idx = url.find("https://");
			if (0 == idx)
				url.replace(0, 8, "");

			idx = url.find("www.");
			if (0 == idx)
				url.replace(0, 4, "");

			idx = url.find("/");
			if(idx != std::string::npos)
			{
				URLSUFFIX = url.substr(idx, url.size());
			}
			std::string host = url.substr(0,idx);

			idx = host.find(":");
			if(idx != std::string::npos)
			{
				std::string sport = host.substr(idx+1, host.size());
				PORT = boost::lexical_cast<int>(sport);
			}

			HOST = host.substr(0,idx);
		}
		else
		{
			std::cout << "Url was not set. (it's mandatory argument)"
				  << std::endl
				  << "Program terminated." << std::endl;
#ifndef NDEBUG
getchar();
#endif
			return 0;
		}

        	if (vm.count("layer"))
			LAYER = layer;
		else
		{
			std::cout << "A layer belonging to a wms-service is not specified. (it's mandatory argument)"
				  << std::endl
				  << "Program terminated." << std::endl;
#ifndef NDEBUG
getchar();
#endif
			return 0;
		}

        	if (vm.count("bbox"))
        	{
			boost::tokenizer<boost::escaped_list_separator<char> > tok(sbox); // csv tokenizer !!
			for(boost::tokenizer<boost::escaped_list_separator<char> >::iterator it=tok.begin();
				it!=tok.end(); ++it)
			{
				std::string str(*it);
				boost::trim(str); // trimer !!
				bbox.push_back(boost::lexical_cast<double>(str));
			}

			if(bbox.size() != 4)
				std::cout << "Problem with BBOX. It should have 4 parameters (left, bottom, right, top), but it is not so?!" << std::endl;

			else if(bbox[0] >= bbox[2])
				std::cout << "Problem with BBOX. bbox.left should be less than bbox.right, but it is not so?!" << std::endl
					  << "[--bbox left bottom right top]" << std::endl;

			else if(bbox[1] >= bbox[3])
				std::cout << "Problem with BBOX. bbox.bottom should be less than bbox.top, but it is not so?!" << std::endl
					  << "[--bbox left bottom right top]" << std::endl;

			else
			{
				BBOX_LEFT   = bbox[0];
				BBOX_BOTTOM = bbox[1];
				BBOX_RIGHT  = bbox[2];
				BBOX_TOP    = bbox[3];
			}
        	}
		else
		{
			std::cout << "BBOX is not specified. (it's mandatory argument)"
				  << std::endl
				  << "Program terminated." << std::endl;
#ifndef NDEBUG
getchar();
#endif
			return 0;
		}

        	if (vm.count("res"))
			RESOLUTION = resolution;
		else
		{
			std::cout << "Resolution is not specified. (it's mandatory argument)"
				  << std::endl
				  << "Program terminated." << std::endl;
#ifndef NDEBUG
getchar();
#endif
			return 0;
		}

        	if (vm.count("crs"))
			CRS = crs;

        	if (vm.count("quality"))
			QUALITY = quality;

		if (vm.count("verbose"))
			VERBOSE = true;

        	if(vm.count("background"))
        	{
            		if("white" == background)
                		BGD = 255;
            		else if(background != "black")
            		{
                		std::cout << "Wrong background color" << std::endl
					  << "Background color can be only 'black' or 'white'; '"
					  << background << "' is not allowed."
					  << std::endl
					  << "Program terminated." << std::endl;
#ifndef NDEBUG
getchar();
#endif
                		return 0;
            		}
        	}

		if (vm.count("no-opt"))
			NO_OPTIMIZATION = true;

        	makeHttpTest();

        	std::cout << "TMS tiles are being created. Please, wait..." << std::endl;

		clock_t t = clock();

        	sprintf(OUTPUT_FOLDER, "tms");
		makeTmsComplete();

        	if(VERBOSE)
            		std::cout << std::endl << "Creating sample files: 0%...";

		if(OSGEO)
			makeTmsSampleOL2_strict();
		else
			makeTmsSampleOL2_amended();

        	makeTmsSampleOL3();

		makeWmsCapabilites();
		makeExceptionXmls();
		makeWmsPhp();
		makeWmsSampleOL2();
		makeWmsSampleOL3();
		makeWmsSampleArcGisJSApi();

        	if(VERBOSE)
            		std::cout << " 100% - OK!" << std::endl;

		t = clock() - t;
		double dsec = static_cast<double>(t)/CLOCKS_PER_SEC;
		int imin = static_cast<int>(dsec) / 60;
		dsec -= imin*60;
		int ihour = imin / 60;
		imin %= 60;

		std::cout << std::endl << "The end!" << std::endl;
		std::cout << "TMS Structure has been created in " << ihour << "h " << imin << "m " << dsec << "sec." << std::endl << std::endl;
	}

	catch(const char* message)
	{
		std::cout << message << std::endl;
	}

	catch(const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	catch (...)
	{
		std::cout << "Program terminated due to the unknown and unexpected problem." << std::endl
                  << "Please, refer to the author of this software." << std::endl;
	}

    return 0; /*EXIT_SUCCESS*/
}
