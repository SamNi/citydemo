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

#define PI              (3.141592653589793238462643)

// forward decls
char *readFile(const char *fname);

typedef double Real;

inline Real uniform(Real a, Real b) {
    assert(a <= b);
    return (Real)((b-a)*((double)rand()/RAND_MAX) + a);
}

// Vec3
/*
struct Vec3 {
    Real x, y, z;
    Vec3(void) {}
    Vec3(Real _x, Real _y, Real _z)  : x(_x), y(_y), z(_z) {}
    Vec3(const Vec3& rhs) : x(rhs.x),y(rhs.y),z(rhs.z) {}
};
inline Vec3 operator*(Real lhs, const Vec3& rhs) { return Vec3(lhs*rhs.x,lhs*rhs.y,lhs*rhs.z); }
inline Vec3 operator+(const Vec3& lhs, const Vec3& rhs) { return Vec3(lhs.x+rhs.x, lhs.y+rhs.y, lhs.z+rhs.z); }
inline Vec3 operator-(const Vec3& lhs, const Vec3& rhs) { return Vec3(lhs.x-rhs.x, lhs.y-rhs.y, lhs.z-rhs.z); }
inline Real dot(const Vec3& lhs, const Vec3& rhs) { return (lhs.x*rhs.x)+(lhs.y*rhs.y)+(lhs.z*rhs.z); }
inline Real magnitudeSquared(const Vec3& v) { return (v.x*v.x)+(v.y*v.y)+(v.z*v.z); }
inline Real magnitude(const Vec3& v) { return sqrt(magnitudeSquared(v)); }*/
/*
struct Sphere { 
    Vec3 pos; 
    Real radius; 
    Sphere(void) {}
    Sphere(const Vec3& _pos, const Real& _radius) : pos(_pos),radius(_radius) {}
};*/

#endif // ~_ESSENTIALS_H_