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

#define PI              (3.141592653589793238462643)

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

typedef float Real;

inline Real uniform(Real a, Real b) {
    assert(a <= b);
    return (Real)((b-a)*((double)rand()/RAND_MAX) + a);
}


#endif // ~_ESSENTIALS_H_