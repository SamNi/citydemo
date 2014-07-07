/*
#include "Renderer.h"
#include "GL.H"

void Renderer::Init(int width, int height) {
    cam.eye = Vec3(3, 3, 3);
    cam.lookAt = Vec3(0,0,0);
    cam.up = Vec3(0,1,0);
    cam.fov = 60;

    glClearColor(.2,.2,.2,1.0);
    Reshape(width, height);
}

void Renderer::Reshape(int w, int h) {
    glViewport(0, 0, w, h);
    width=w;
    height=h;
}

void Renderer::Render(const World& s) {
    float ratio = width/(float)height;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(cam.fov, ratio, 0.01, 10.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(cam.eye.x,cam.eye.y,cam.eye.z,
        cam.lookAt.x,cam.lookAt.y,cam.lookAt.z,
        cam.up.x,cam.up.y,cam.up.z);

    glPointSize(3.0f);
    glBegin(GL_LINE_LOOP);
    for(unsigned int i=0;i<s.elements.size();++i)
        glVertex3d(s.elements[i].pos.x, s.elements[i].pos.y, s.elements[i].pos.z);
    glEnd();
}

*/