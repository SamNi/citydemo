#include "World.h"

const int nElems = 2500;

Scene::Scene(void) {
    elements.reserve(nElems);
    for(int i=0;i<nElems;++i) {
        Vec3 v(uniform(-1,1), uniform(-1,1), uniform(-1,1));
        elements.push_back(Sphere(v, 1.0));
    }
}

void Scene::step(Real dt) {
    for (int i=0;i<elements.size();++i)
        elements[i].pos = uniform(0.99,1.01)*elements[i].pos;
}