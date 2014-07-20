#include "Texture.h"

#include <zlib.h>
#include <png.h>

Texture::Texture(void) : pixels(nullptr), width(8), height(8), texID(0) {
    MakeCheckerboard();
    UpGL(GL_RGB);
}

Texture::Texture(int w, int h) : pixels(nullptr), width(w), height(h), texID(0) {
    assert(w && h);
    MakeCheckerboard();
    UpGL(GL_RGB); 
}

Texture::Texture(char *fname) : pixels(nullptr), width(8), height(8), texID(0) {
    png_image img;

    memset(&img, 0, sizeof(img));

    img.version = PNG_IMAGE_VERSION;

    if (!png_image_begin_read_from_file(&img, fname)) {
        fprintf(stderr, "couldn't read %s\n", fname);
        MakeCheckerboard();
        UpGL(GL_RGB);
        return;
    }

    if (!Alloc(PNG_IMAGE_SIZE(img))) {
        fprintf(stderr, "error allocing buf for %s\n", fname);
        MakeCheckerboard();
        UpGL(GL_RGB);
        return;
    }

    if (!png_image_finish_read(&img, NULL, pixels, 0, NULL)) {
        fprintf(stderr, "png_image_finish_read failed on %s\n", fname);
        MakeCheckerboard();
        UpGL(GL_RGB);
        return;
    }
    width = img.width;
    height = img.height;

    GLuint fmt;
    switch(img.format) {
    case PNG_FORMAT_RGB:
        fmt = GL_RGB;
        break;
    case PNG_FORMAT_RGBA:
        fmt = GL_RGBA;
        break;
    case PNG_FORMAT_BGR:
        fmt = GL_BGR;
    case PNG_FORMAT_BGRA:
        fmt = GL_BGRA;
    default:
        fmt = GL_RED;
    }
    UpGL(fmt);
}

Texture::~Texture(void) {
    DeAlloc();
    glDeleteTextures(1, &texID);
}

void Texture::Bind(void) const { glBindTexture(GL_TEXTURE_2D, texID); }
void Texture::Refresh(void) {
    UpGL(GL_RGB);
}

GLuint Texture::getTexID(void) const {
    return texID;
}

uint8_t *Texture::getPixels(void) const {
    return pixels;
}

void Texture::UpGL(GLuint fmt) {
    if (0 == texID)
        glGenTextures(1, &texID);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, fmt, GL_UNSIGNED_BYTE, pixels);
    glBindImageTexture(0, texID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
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

void Texture::MakeCheckerboard(void) {
    static int i, j, addr;
    static bool hot;

    Alloc(width, height);

    assert(pixels);
    if (!pixels)
        return;

    hot = false;
    for (i = 0;i < width;++i) {
        for (j = 0;j < height;++j, hot = !hot) {
            addr = 3*(width*j + i);
            if (hot) {
                pixels[addr+0] = 255;
                pixels[addr+1] = 174;
                pixels[addr+2] = 75;
            } else {
                pixels[addr+0] = 38;
                pixels[addr+1] = 38;
                pixels[addr+2] = 38;
            }
        }
        hot = !hot;
    }
}