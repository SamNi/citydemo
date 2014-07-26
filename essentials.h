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

// forward decls
// caller's responsibility to free the returned buf
char *readFile(const char *fname);
void checkGL(void);

typedef float Real;

inline Real uniform(Real a, Real b) {
    assert(a <= b);
    return (Real)((b-a)*((double)rand()/RAND_MAX) + a);
}


#endif // ~_ESSENTIALS_H_