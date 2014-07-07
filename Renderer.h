/*
#ifndef _RENDERER_H_
#define _RENDERER_H_
#include "essentials.h"
#include "World.h"

struct Pix2 {
    uint16_t x, y;
    Pix2() {}
    Pix2(uint16_t _x, uint16_t _y) : x(_x),y(_y) { }
};

// Viewport (screen pixel space)
struct Viewport {
    Pix2 origin, dims;
};

struct Camera {
    Vec3 eye, lookAt, up;
    uint16_t fov;
    Viewport vp;
};

struct Renderer {
    void Init(int w, int h);
    void Reshape(int w, int h);
    void Render(const World& s);

    int width, height;
    bool fullscreen;
    Camera cam;
};

#endif // ~_RENDERER_H_
*/