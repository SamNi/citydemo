// Copyright 2014 SamNi PlaceholderLicenseText
#ifndef _BACKEND_H_
#define _BACKEND_H_
#include "essentials.h"
// Think about:
// what kind of low level commands might my backend render queue provide?
// you might not need much! consider putting the smarts in the front end
// back end just optimizes and reorders to minimize state changes)
//
//      refresh (reread a texture image from disk and reupload to the GL)
//      flush, to be called at the end of a frame
// what would be the most potentially often changed state?
// what kind of statistics would I like to keep to aid in optimization?
//      per frame?
//      per last few seconds?
//      since program startup?
// do I really want to double buf anything?
//      and if so, what?

typedef glm::u8vec3 RGB;
typedef glm::u8vec4 RGBA;
typedef glm::u16vec2 TexCoord;
typedef uint32_t PackedNormal;

class Backend {
public:
    static bool     startup(int w, int h);
    static void     begin_frame(void);
    static void     end_frame(void);
    static void     shutdown(void);

    static void     resize(int w, int h);
    static RGB*     get_screenshot(void);
    static void     write_screenshot(void);
    static bool     write_screenshot(const char *fname);
    static void     add_tris(void);

    static void     set_modelview(const glm::mat4x4& m);
    static void     set_projection(const glm::mat4x4& m);

    static void     enable_depth_testing(void);
    static void     disable_depth_testing(void);
    static void     enable_blending(void);
    static void     enable_additive_blending(void);
    static void     disable_blending(void);

private:
    struct Impl;
    static std::unique_ptr<Impl> mImpl;
};
#endif // ~_BACKEND_H_