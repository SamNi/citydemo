#ifndef _TEXTURE_H_
#define _TEXTURE_H_
#include "essentials.h"
#include "GL.H"

// Image : API-independent
// Texture : Everything needed to function with OpenGL + the associated image

enum ImageFormat {
    RGB,
    RGBA,
    GRAYSCALE
};

struct Image {
    int w, h;
    uint8_t *pixels;
    ImageFormat fmt;
};

struct Texture {
    Texture(void);
    Texture(int w, int h);
    Texture(const char *fname, bool filtered = true, bool mipmapped = true);
    ~Texture(void);

    void Bind(void) const;
    void Refresh(void);

    uint8_t *getPixels(void) const;
    GLuint getTexID(void) const;
    const char *getName(void) const;
    size_t getSizeInBytes(void) const;

    int nComponents;
    std::string path;
    GLuint texID;
    GLuint sizedFormat, baseFormat;
    bool bFilter;
    bool bUseMipmaps;

    Image img;

private:
    bool Alloc(int nbytes);
    bool Alloc(int w, int h);
    void DeAlloc(void);
    void UpGL();
    void MakeCheckerboard(void);
};

#endif // ~_TEXTURE_H_