#ifndef FRONTEND_H_
#define FRONTEND_H_
#include "../../essentials.h"
#include "../../GLM.h"

class Frontend {
public:
    static bool startup(int w, int h);
    static void strafe(const glm::vec3& dir);
    static void mouselook(float delta_x, float delta_y);
    static void move(const glm::vec3& dir);
    static void render();
    static void shutdown(void);

    static void write_screenshot(void);

    static void change_fov(float fov);

private:
    struct Impl;
    static std::unique_ptr<Impl> mImpl;
};

#endif  // FRONTEND_H_