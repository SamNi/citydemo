// Represents the state of the world. The M in MVC

#ifndef _SCENE_H_
#define _SCENE_H_
#include "essentials.h"

struct Scene {
    std::vector<Sphere> elements;

    Scene(void);
    void step(Real dt);
};

#endif // ~_SCENE_H_