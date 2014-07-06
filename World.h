// Represents the state of the world. The M in MVC

#ifndef _WORLD_H_
#define _WORLD_H_
#include "essentials.h"

struct Scene {
    std::vector<Sphere> elements;

    Scene(void);
    void step(Real dt);
};

#endif // ~_WORLD_H_