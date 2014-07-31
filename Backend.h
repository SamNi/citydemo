// Copyright 2014 SamNi PlaceholderLicenseText
#ifndef _BACKEND_H_
#define _BACKEND_H_
#include "essentials.h"
namespace Backend {

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

bool Startup(int w, int h);
void BeginFrame(void);
void EndFrame(void);
void Shutdown(void);
void Resize(int w, int h);
void Screenshot(void);
void DrawFullscreenQuad(void);

}
#endif // ~_BACKEND_H_