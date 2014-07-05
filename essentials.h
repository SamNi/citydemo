#ifndef _ESSENTIALS_H_
#define _ESSENTIALS_H_
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cassert>
#include <cmath>

#include <list>
#include <vector>

typedef double Real;

struct Vec3 {
    Real x, y, z;
    Vec3(void) {}
    Vec3(Real _x, Real _y, Real _z)  : x(_x), y(_y), z(_z) {}
    Vec3(const Vec3& rhs) : x(rhs.x),y(rhs.y),z(rhs.z) {}
};
struct Sphere { 
    Vec3 pos; Real radius; 
    Sphere(void) {}
    Sphere(const Vec3& _pos, const Real& _radius) : pos(_pos),radius(_radius) {}
};

inline Real uniform(Real a, Real b) {
    assert(a <= b);
    return (Real)((b-a)*((double)rand()/RAND_MAX) + a);
}

#endif // ~_ESSENTIALS_H_