// TODO:
// - 8-bit pngs are paletted and require special consideration.
// The intent is to ignore the palette entirely and treat it as grayscale
//
// - I get an invalid enumerant when I try to do compressed textures. fix that
#include "Texture.h"

#include <zlib.h>
#include <png.h>

Texture::Texture(void) : pixels(nullptr), width(8), height(8), texID(0), bFilter(true), bUseMipmaps(false) {
    MakeCheckerboard();
}

Texture::Texture(int w, int h) : pixels(nullptr), width(w), height(h), texID(0), bFilter(true), bUseMipmaps(false) {
    assert(w && h);
    MakeCheckerboard();
}

Texture::Texture(char *fname, bool filtered, bool mipmapped) : pixels(nullptr), width(8), height(8), texID(0), bFilter(filtered), bUseMipmaps(mipmapped) {
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
    default:
        fprintf(stderr, "Unsupported format\n");
        MakeCheckerboard();
        break;
    }
    UpGL();
    DeAlloc();
}

Texture::~Texture(void) {
    DeAlloc();
    glDeleteTextures(1, &texID);
}

void Texture::Bind(void) const {
    glBindTexture(GL_TEXTURE_2D, texID);
}
void Texture::Refresh(void) {
    UpGL();
}

GLuint Texture::getTexID(void) const {
    return texID;
}

const char *Texture::getName(void) const { return this->path.c_str(); }

size_t Texture::getSizeInBytes(void) const {
    if (pixels)
        return width*height*nComponents + sizeof(Texture);
    else
        return 0;
}

uint8_t *Texture::getPixels(void) const {
    return pixels;
}

static inline double log2(double x) { return log(x)/log(2); }
static inline int ComputeMipmapLevel(int w, int h) { return (int)ceil(log2(max(w, h))); }

void Texture::UpGL() {
    // Reinterprets RG to grayscale alpha as intended for those that need it

    if (0 == texID)
        glGenTextures(1, &texID);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);

    GLuint internalFormat;

    internalFormat = GL_RGBA8;

    // The old way: Mutable storage. 
    // glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, this->format, GL_UNSIGNED_BYTE, pixels);
    // glBindImageTexture(0, texID, 0, GL_FALSE, 0, GL_WRITE_ONLY, internalFormat);
    // 
    // The new way: Immutable storage.
    int nMipmaps = bUseMipmaps ? ComputeMipmapLevel(width, height) : 1;
    checkGL();
    glTexStorage2D(GL_TEXTURE_2D, nMipmaps, internalFormat, width, height);
    checkGL();
    glBindImageTexture(0, texID, 0, GL_FALSE, 0, GL_WRITE_ONLY, internalFormat);
    checkGL();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, pixels);
    checkGL();
    if (bUseMipmaps) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, bFilter ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, bFilter ? GL_LINEAR : GL_NEAREST);
    }
    checkGL();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, bFilter ? GL_LINEAR : GL_NEAREST);
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

// Useful placeholder in the case when textures don't
// load for whatever reason
void Texture::MakeCheckerboard(void) {
    static int i, j, addr;
    static bool hot;

    nComponents = 4;
    Alloc(nComponents*width*height*sizeof(uint8_t));
    checkGL();

    assert(pixels);
    if (!pixels)
        return;

    this->format = GL_RGBA;
    bFilter = false;
    bUseMipmaps = false;
    hot = false;
    for (j = 0;j < height;++j) {
        for (i = 0;i < width;++i, hot = !hot) {
            addr = (width*j + i)*nComponents;
            if (hot) {
                pixels[addr+0] = 255;
                pixels[addr+1] = 0;
                pixels[addr+2] = 255;
                pixels[addr+3] = 255;
            } else {
                pixels[addr+0] = 0;
                pixels[addr+1] = 255;
                pixels[addr+2] = 0;
                pixels[addr+3] = 64;
            }
        }
        hot = !hot;
    }
    UpGL();
}
