// Copyright [year] <Copyright Owner>
#ifndef _ESSENTIALS_H_
#define _ESSENTIALS_H_
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cassert>
#include <cmath>
#include <cstdint>

#include <list>
#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include <set>
#include <map>
#include <memory>

#include "GLM.h"

#define PI              (3.141592653589793238462643)

typedef glm::u8vec3 RGBPixel;
typedef glm::u8vec4 RGBAPixel;
typedef glm::u16vec2 TexCoord;
typedef uint32_t PackedNormal;

enum PixelColor {
    // look up the original VGA 16-color palette (4-bit RGBI) to see where these came from 
    BLACK = 0,
    DARK_BLUE, DARK_GREEN, DARK_CYAN, 
    DARK_RED, DARK_PURPLE, BROWN, GRAY, 
    DARK_GRAY, BLUE, GREEN, CYAN,
    RED, PURPLE, YELLOW, WHITE,
    FINAL_FANTASY, // old habits die hard
};
const RGBPixel COLOR[] = {
    RGBPixel(0,0,0), RGBPixel(0,0,127), RGBPixel(0,127,0), RGBPixel(0,127,127),
    RGBPixel(127,0,0), RGBPixel(127,0,127), RGBPixel(127,85,0), RGBPixel(127,127,127), 
    RGBPixel(85, 85, 85), RGBPixel(85,85,255), RGBPixel(85,255,85), RGBPixel(85,255,255), 
    RGBPixel(255,85,85), RGBPixel(255,85,255), RGBPixel(255,255,85), RGBPixel(255,255,255), 
    RGBPixel(0, 0, 136),
};
const RGBAPixel COLOR_ALPHA[] = {
    RGBAPixel(0,0,0, 255), RGBAPixel(0,0,127, 255), RGBAPixel(0,127,0, 255), RGBAPixel(0,127,127, 255),
    RGBAPixel(127,0,0, 255), RGBAPixel(127,0,127, 255), RGBAPixel(127,85,0, 255), RGBAPixel(127,127,127, 255), 
    RGBAPixel(85, 85, 85, 255), RGBAPixel(85,85,255, 255), RGBAPixel(85,255,85, 255), RGBAPixel(85,255,255, 255), 
    RGBAPixel(255,85,85, 255), RGBAPixel(255,85,255, 255), RGBAPixel(255,255,85, 255), RGBAPixel(255,255,255, 255), 
    RGBAPixel(0, 0, 136, 255),
};
const int NUM_COLORS = sizeof(COLOR) / sizeof(RGBPixel);

#ifdef _WIN32
#define PATH_SEP                    "\\"
#else
#define PATH_SEP                    "/"
#endif

enum LogLevel {
    LOG_CRITICAL = 0,
    LOG_WARNING = 1,
    LOG_INFORMATION = 2,
    LOG_VERBOSE = 3,
    LOG_TRACE = 4,
};

#define LOG(l, msg, ...)             _log((l), __FILE__, __LINE__, __FUNCTION__,(msg), __VA_ARGS__)


// forward decls
// caller's responsibility to free the returned buf
char *readFile(const char *fname);
void _log(LogLevel l, const char *srcFile, int lineNo, const char *funcName, const char *msg, ...);
void checkGL(void);
void imgflip(int w, int h, int nComponents, uint8_t *pixels);
// for use with GL_UNSIGNED_INT_2_10_10_10_REV
PackedNormal normal_pack(const glm::vec4& v);

typedef float Real;

inline Real uniform(Real a, Real b) {
    assert(a <= b);
    return (Real)((b-a)*((double)rand()/RAND_MAX) + a);
}


#endif // ~_ESSENTIALS_H_