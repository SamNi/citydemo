#include "Scene.h"
#include <Windows.h>
#include <GL/GL.h>
#include <GL/GLU.h>

const int nElems = 50000;

Scene::Scene(void) {
    elements.reserve(nElems);
    for(int i=0;i<nElems;++i) {
        Vec3 v(uniform(-1.f,1),uniform(-.5f,.5f),uniform(-1,1));
        Real r = 1.0;
        elements.push_back(Sphere(v, r));
    }
}

void Scene::draw(void) const {
    glPointSize(5.0f);

    glBegin(GL_POINTS);
    for(int i=0;i<elements.size();++i)
        glVertex3f(elements[i].pos.x, elements[i].pos.y, elements[i].pos.z);
    glEnd();
}

void Scene::step(void) {
}