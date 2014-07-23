// TODO:
// - 8-bit pngs are paletted and require special consideration.
// The intent is to ignore the palette entirely and treat it as grayscale
//
// - I get an invalid enumerant when I try to do compressed textures. fix that
#include "Texture.h"

#include <zlib.h>
#include <png.h>

Texture::Texture(void) : pixels(nullptr), width(8), height(8), texID(0), filter(true), bUseMipmaps(false) {
    MakeCheckerboard();
    UpGL();
}

Texture::Texture(int w, int h) : pixels(nullptr), width(w), height(h), texID(0), filter(true), bUseMipmaps(false) {
    assert(w && h);
    MakeCheckerboard();
    UpGL(); 
}

Texture::Texture(char *fname, bool filtered, bool Mipmapped) : pixels(nullptr), width(8), height(8), texID(0), filter(filtered), bUseMipmaps(Mipmapped) {
    png_image img;

    path = std::string(fname);
    memset(&img, 0, sizeof(img));
    img.version = PNG_IMAGE_VERSION;

    if (!png_image_begin_read_from_file(&img, fname)) {
        fprintf(stderr, "couldn't read %s\n", fname);
        MakeCheckerboard();
        path = path + std::string(" COULD NOT READ ");
        return;
    }

    if (!Alloc(PNG_IMAGE_SIZE(img))) {
        fprintf(stderr, "error allocing buf for %s\n", fname);
        MakeCheckerboard();
        path = path + std::string(" COULD NOT READ ");
        return;
    }

    if (!png_image_finish_read(&img, NULL, pixels, 0, NULL)) {
        fprintf(stderr, "png_image_finish_read failed on %s\n", fname);
        MakeCheckerboard();
        path = path + std::string(" COULD NOT READ ");
        return;
    }
    
    width = img.width;
    height = img.height;

    // TODO: grayscale with alpha requires special consideration due to the
    // deprecation of GL_LUMINANCE in OpenGL v4.x
    switch(img.format) {
    case PNG_FORMAT_RGB:
        this->format = GL_RGB;
        nComponents = 3;
        break;
    case PNG_FORMAT_RGBA:
        this->format = GL_RGBA;
        nComponents = 4;
        break;
    case PNG_FORMAT_GRAY:
        this->format = GL_RED;
        nComponents = 1;
    default:
        fprintf(stderr, "Unsupported format\n");
        MakeCheckerboard();
        break;
    }
    UpGL();
}

Texture::~Texture(void) {
    DeAlloc();
    glDeleteTextures(1, &texID);
}

void Texture::Bind(void) const { glBindTexture(GL_TEXTURE_2D, texID); }
void Texture::Refresh(void) {
    UpGL();
}

GLuint Texture::getTexID(void) const {
    return texID;
}

const char *Texture::getName(void) const { return this->path.c_str(); }

size_t Texture::getSizeInBytes(void) const {
    assert(pixels);
    return width*height*nComponents + sizeof(Texture);
}

uint8_t *Texture::getPixels(void) const {
    return pixels;
}

static inline double log2(double x) { return log(x)/log(2); }
static inline int ComputeMipmapLevel(int w, int h) { return (int)ceil(log2(max(w, h))); }

void Texture::UpGL() {
    // Reinterprets RG to grayscale alpha as intended
    const static GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_GREEN };

    if (0 == texID)
        glGenTextures(1, &texID);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);

    GLuint internalFormat;
    switch (this->format) {
        
    case GL_RGBA:
    case GL_RGB:
        internalFormat = GL_RGBA8;
        break;
    case GL_RED:
        internalFormat = GL_R8;
        break;
    case GL_RG:
        internalFormat = GL_RG8;
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
        break;
    default:
        internalFormat = 0xDEADBEEF;    // shut compiler warning up
        assert(!"this shouldn't happen");
    }

    // The old way: Mutable storage. 
    // glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, this->format, GL_UNSIGNED_BYTE, pixels);
    // glBindImageTexture(0, texID, 0, GL_FALSE, 0, GL_WRITE_ONLY, internalFormat);
    // 
    // The new way: Immutable storage.
    int nMipmaps = bUseMipmaps ? ComputeMipmapLevel(width, height) : 1;
    glTexStorage2D(GL_TEXTURE_2D, nMipmaps, internalFormat, width, height);
    glBindImageTexture(0, texID, 0, GL_FALSE, 0, GL_WRITE_ONLY, internalFormat);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, this->format, GL_UNSIGNED_BYTE, pixels);
    checkGL();
    if (bUseMipmaps) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter ? GL_LINEAR : GL_NEAREST);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    checkGL();
}

bool Texture::Alloc(int w, int h) { return Alloc(3*w*h); }

bool Texture::Alloc(int nbytes) {
    DeAlloc();
    pixels = new uint8_t[nbytes];
    assert(pixels);
    if (!pixels) {
        fprintf(stderr, "Texture::Alloc failed (%d bytes)\n", nbytes);
        return false;
    }
    return true;
}

void Texture::DeAlloc(void) {
    if (!pixels)
        return;
    delete[] pixels;
    pixels = nullptr;
}

// Generate 8x8 grayscale alpha checkerboard
// Useful placeholder in the case when textures don't
// load for whatever reason
void Texture::MakeCheckerboard(void) {
    static int i, j, addr;
    static bool hot;

    nComponents = 2;
    Alloc(nComponents*width*height);

    assert(pixels);
    if (!pixels)
        return;

    format = GL_RG;
    filter = false;

    hot = false;
    for (i = 0;i < width;++i) {
        for (j = 0;j < height;++j, hot = !hot) {
            addr = nComponents*(width*j + i);
            if (hot) {
                pixels[addr+0] = 192;
                pixels[addr+1] = 192;
            } else {
                pixels[addr+0] = 51;
                pixels[addr+1] = 127;
            }
        }
        hot = !hot;
    }
    UpGL();
}
