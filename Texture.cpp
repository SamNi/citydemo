// TODO:
// - 8-bit pngs are paletted and require special consideration.
// The intent is to ignore the palette entirely and treat it as grayscale
//
// - I get an invalid enumerant when I try to do compressed textures. fix that
#include "Texture.h"

#include <zlib.h>
#include <png.h>
#include <physfs/physfs.h>

Texture::Texture(void) : texID(0), bFilter(true), bUseMipmaps(false) {
    img.w = 8;
    img.h = 8;
    img.pixels = nullptr;
    MakeCheckerboard();
}

Texture::Texture(int w, int h) : texID(0), bFilter(true), bUseMipmaps(false) {
    assert(w && h);
    img.w = w;
    img.h = h;
    img.pixels = nullptr;
    MakeCheckerboard();
}

Texture::Texture(const char *fname, bool filtered, bool mipmapped) : texID(0), bFilter(filtered), bUseMipmaps(mipmapped) {
    img.w = 8;
    img.h = 8;
    img.pixels = nullptr;

    // phyfs
    PHYSFS_file *fin = nullptr;
    PHYSFS_sint64 fileLen;
    uint8_t *buf = nullptr;
    fin = PHYSFS_openRead(fname);

    if (nullptr == fin)
        goto img_err;

    fileLen = PHYSFS_fileLength(fin);
    buf = new uint8_t[fileLen+1];
    PHYSFS_read(fin, buf, 1, fileLen);
    buf[fileLen] = '\0';            // ~physfs

    // libpng
    png_image img;
    path = std::string(fname);
    memset(&img, 0, sizeof(img));
    img.version = PNG_IMAGE_VERSION;

    if (!png_image_begin_read_from_memory(&img, buf, (png_size_t)fileLen))
        goto img_err;

    if (!Alloc(PNG_IMAGE_SIZE(img)))
        goto img_err;

    if (!png_image_finish_read(&img, NULL, this->img.pixels, 0, nullptr))
        goto img_err;

    delete[] buf;

    // TODO: name shadows are confusing
    this->img.w = img.width;
    this->img.h = img.height;

    // TODO: grayscale with alpha requires special consideration due to the
    // deprecation of GL_LUMINANCE in OpenGL v4.x
    switch(img.format) {
    case PNG_FORMAT_RGB:
        this->img.fmt = ImageFormat::RGB;
        nComponents = 3;
        break;
    case PNG_FORMAT_RGBA:
        this->img.fmt = ImageFormat::RGBA;
        nComponents = 4;
        break;
    default:
        fprintf(stderr, "Unsupported format\n");
        MakeCheckerboard();
        break;
    }
    UpGL();
    DeAlloc();
    return;

    // GOTO considered harmful
img_err:
    delete[] buf;
    if (fin)
        PHYSFS_close(fin);
    fprintf(stderr, "couldn't read %s\n", fname);
    MakeCheckerboard();
    path = path + std::string("COULD NOT READ");
    return;
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
    if (img.pixels)
        return img.w*img.h*nComponents + sizeof(Texture);
    else
        return 0;
}

uint8_t *Texture::getPixels(void) const {
    return img.pixels;
}

static inline double log2(double x) { return log(x)/log(2); }
static inline int ComputeMipmapLevel(int w, int h) { return (int)ceil(log2(max(w, h))); }

void Texture::UpGL() {
    // Reinterprets RG to grayscale alpha as intended for those that need it

    if (0 == texID)
        glGenTextures(1, &texID);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);

    // Note the distinction between our app's enum and the GL constants
    switch(img.fmt) {
    case ImageFormat::RGB:
        baseFormat = GL_RGB;
        sizedFormat = GL_RGBA8;
        break;
    case ImageFormat::RGBA:
        baseFormat = GL_RGBA;
        sizedFormat = GL_RGBA8;
        break;
    case ImageFormat::GRAYSCALE:
        baseFormat = GL_RED;
        sizedFormat = GL_RGBA8;
        break;
    default:
        assert(!"I don't know this image format");
        break;
    }

    // The old way: Mutable storage. 
    // glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, this->format, GL_UNSIGNED_BYTE, pixels);
    // glBindImageTexture(0, texID, 0, GL_FALSE, 0, GL_WRITE_ONLY, internalFormat);
    // 
    // The new way: Immutable storage.
    int nMipmaps = bUseMipmaps ? ComputeMipmapLevel(img.w, img.h) : 1;
    checkGL();
    glTexStorage2D(GL_TEXTURE_2D, nMipmaps, sizedFormat, img.w, img.h);
    checkGL();
    glBindImageTexture(0, texID, 0, GL_FALSE, 0, GL_WRITE_ONLY, sizedFormat);
    checkGL();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, img.w, img.h, baseFormat, GL_UNSIGNED_BYTE, img.pixels);
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
    img.pixels = new uint8_t[nbytes];
    assert(img.pixels);
    if (!img.pixels) {
        fprintf(stderr, "Texture::Alloc failed (%d bytes)\n", nbytes);
        return false;
    }
    return true;
}

void Texture::DeAlloc(void) {
    if (!img.pixels)
        return;
    delete[] img.pixels;
    img.pixels = nullptr;
}

// Useful placeholder in the case when textures don't
// load for whatever reason
void Texture::MakeCheckerboard(void) {
    static int i, j, addr;
    static bool hot;

    nComponents = 4;
    Alloc(nComponents*img.w*img.h*sizeof(uint8_t));
    checkGL();

    assert(img.pixels);
    if (!img.pixels)
        return;

    this->img.fmt = ImageFormat::RGBA;
    bFilter = false;
    bUseMipmaps = false;
    hot = false;
    for (j = 0;j < img.h;++j) {
        for (i = 0;i < img.w;++i, hot = !hot) {
            addr = (img.w*j + i)*nComponents;
            if (hot) {
                img.pixels[addr+0] = 255;
                img.pixels[addr+1] = 0;
                img.pixels[addr+2] = 255;
                img.pixels[addr+3] = 255;
            } else {
                img.pixels[addr+0] = 0;
                img.pixels[addr+1] = 255;
                img.pixels[addr+2] = 0;
                img.pixels[addr+3] = 64;
            }
        }
        hot = !hot;
    }
    UpGL();
}

