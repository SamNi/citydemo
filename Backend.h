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

class Backend {
public:
    static bool startup(int w, int h);
    static void begin_frame(void);
    static void end_frame(void);
    static void shutdown(void);
    static void resize(int w, int h);
    static void screenshot(void);
    static void draw_fullscreen_quad(void);
    static void add_tris(void);

private:
    struct Impl;
    static std::unique_ptr<Impl> mImpl;
};
#endif // ~_BACKEND_H_