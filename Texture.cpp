#include "Texture.h"

Texture::Texture(int w, int h) : width(w),height(h) {
    assert(w && h);

    pixels = new uint8_t[w*h*3];
    
    for (int i = 0;i < w*h;++i) {
        int factor = glm::max(w,h) / 6;
        uint8_t c = (uint8_t)(255.0/factor)*(i%factor);
        pixels[3*i+0] = 255-c;
        pixels[3*i+1] = 255-c;
        pixels[3*i+2] = 255-c;
    }

    glGenTextures(1, &texID);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
}

Texture::~Texture(void) {
    if (pixels)
        delete[] pixels;
    glDeleteTextures(1, &texID);
}

void Texture::Use(void) const {
    glBindTexture(GL_TEXTURE_2D, texID);
}

GLuint Texture::getTexID(void) const {
    return texID;
}

uint8_t *Texture::getPixels(void) const {
    return pixels;
}