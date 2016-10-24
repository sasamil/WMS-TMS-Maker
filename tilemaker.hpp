
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

