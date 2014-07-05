#ifndef _SCENE_H_
#define _SCENE_H_
#include "essentials.h"

struct Scene {
    std::vector<Sphere> elements;

    Scene(void);
    void draw(void) const;
    void step(void);
};

#endif // ~_SCENE_H_