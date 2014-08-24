// Copyright [year] <Copyright Owner>
// TODO(SamNi):
// - 8-bit pngs are paletted and require special consideration.
// The intent is to ignore the palette entirely and treat it as grayscale
//
// - I get an invalid enumerant when I try to do compressed textures. fix that
#include <zlib.h>
#include <png.h>
#include <physfs/physfs.h>
#include "GL.H"
#include "Backend.h"

struct Texture::Impl {
    Impl(void) : m_texture_id(0), bFilter(true), m_use_mipmaps(false) {
        img.w = 8;
        img.h = 8;
        img.pixels = nullptr;
        MakeCheckerboard();
    }
    Impl(int w, int h) : m_texture_id(0), bFilter(true), m_use_mipmaps(false) {
        assert(w && h);
        img.w = w;
        img.h = h;
        img.pixels = nullptr;
        MakeCheckerboard();
    }
    Impl(const char *fname, bool filtered, bool mipmapped) : 
        m_texture_id(0), 
        bFilter(filtered), 
        m_use_mipmaps(mipmapped) 
    {
        img.w = 8;
        img.h = 8;
        img.pixels = nullptr;

        // phyfs
        PHYSFS_File *fin = nullptr;
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

        if (!png_image_finish_read(&img, nullptr, this->img.pixels, 0, nullptr))
            goto img_err;


        // TODO(SamNi): name shadows are confusing
        this->img.w = img.width;
        this->img.h = img.height;

        // TODO(SamNi): grayscale with alpha requires special consideration due to the
        // deprecation of GL_LUMINANCE in OpenGL v4.x
        switch (img.format) {
        case PNG_FORMAT_RGB:
            this->img.fmt = ImageFormat::RGB_;
            nComponents = 3;
            break;
        case PNG_FORMAT_RGBA:
            this->img.fmt = ImageFormat::RGBALPHA;
            nComponents = 4;
            break;
        default:
            LOG(LOG_WARNING, "Unsupported format\n");
            MakeCheckerboard();
            break;
        }

        imgflip(this->img.w, this->img.h, nComponents, this->img.pixels);
        upload_to_gl();

        DeAlloc();
        delete[] buf;
        png_image_free(&img);
        PHYSFS_close(fin);
        return;

        // GOTO considered harmful
    img_err:
        delete[] buf;
        if (fin)
            PHYSFS_close(fin);
        LOG(LOG_WARNING, "couldn't read %s\n", fname);
        MakeCheckerboard();
        path = path + std::string("COULD NOT READ");
        return;
    }
    ~Impl(void) {
        DeAlloc();
        glDeleteTextures(1, &m_texture_id);
    }
    void bind(void) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_texture_id);
    }
    void refresh(void) { upload_to_gl(); }
    uint32_t get_texture_id(void) const {
        return m_texture_id;
    }
    const char *get_name(void) const { return path.c_str(); } 
    size_t get_size_in_bytes(void) const { return (img.pixels) ? img.w*img.h*nComponents + sizeof(Texture) : 0; }
    uint8_t *get_pixels(void) const { return img.pixels; }
    inline int compute_mipmap_level(int w, int h) { return static_cast<int>(ceil(glm::log2<float>(glm::max(w, h)))); }
    void upload_to_gl(void) {
        // Reinterprets RG to grayscale alpha as intended for those that need it

        if (0 == m_texture_id)
            glGenTextures(1, &m_texture_id);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_texture_id);

        // Note the distinction between our app's enum and the GL constants
        switch (img.fmt) {
        case ImageFormat::RGB_:
            baseFormat = GL_RGB;
            sizedFormat = GL_RGBA8;
            break;
        case ImageFormat::RGBALPHA:
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

        int nMipmaps = m_use_mipmaps ? compute_mipmap_level(img.w, img.h) : 1;
        checkGL();
        glTexStorage2D(GL_TEXTURE_2D, nMipmaps, sizedFormat, img.w, img.h);
        checkGL();
        checkGL();
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, img.w, img.h, baseFormat, GL_UNSIGNED_BYTE, img.pixels);
        checkGL();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        if (m_use_mipmaps) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, bFilter ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST);
            glGenerateMipmap(GL_TEXTURE_2D);
        } else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, bFilter ? GL_LINEAR : GL_NEAREST);
        }
        checkGL();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, bFilter ? GL_LINEAR : GL_NEAREST);
        checkGL();
    }
    bool Alloc(int w, int h) { return Alloc(3*w*h); }
    bool Alloc(int nbytes) {
        DeAlloc();
        img.pixels = new uint8_t[nbytes];
        assert(img.pixels);
        if (!img.pixels) {
            LOG(LOG_CRITICAL, "Texture::Alloc failed (%d bytes)\n", nbytes);
            return false;
        }
        return true;
    }
    void DeAlloc(void) {
        if (!img.pixels)
            return;
        delete[] img.pixels;
        img.pixels = nullptr;
    }
    void MakeCheckerboard(void) {
        static int i, j, addr;
        static bool hot;

        nComponents = 4;
        Alloc(nComponents*img.w*img.h*sizeof(uint8_t));
        checkGL();

        assert(img.pixels);
        if (!img.pixels)
            return;

        this->img.fmt = ImageFormat::RGBALPHA;
        bFilter = false;
        m_use_mipmaps = false;
        hot = false;
        for (j = 0; j < img.h; ++j) {
            for (i = 0; i < img.w; ++i, hot = !hot) {
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
        upload_to_gl();
    }
    int nComponents;
    std::string path;
    uint32_t m_texture_id;
    uint32_t sizedFormat, baseFormat;
    bool bFilter;
    bool m_use_mipmaps;

    Image img;
};

Texture::Texture(void) { m_impl = std::unique_ptr<Impl>(new Impl()); }
Texture::Texture(int w, int h) { m_impl = std::unique_ptr<Impl>(new Impl(w, h)); }
Texture::Texture(const char *fname, bool filtered, bool mipmapped) { m_impl = std::unique_ptr<Impl>(new Impl(fname, filtered, mipmapped)); }
Texture::~Texture(void) { m_impl.reset(nullptr); }
void Texture::bind(void) const { m_impl->bind(); }
void Texture::refresh(void) { m_impl->refresh(); }
GLuint Texture::get_texture_id(void) const { return m_impl->get_texture_id(); }
const char *Texture::get_name(void) const { return m_impl->get_name(); }
size_t Texture::get_size_in_bytes(void) const { return m_impl->get_size_in_bytes(); }
uint8_t *Texture::get_pixels(void) const { return m_impl->get_pixels(); }
double log2(double x) { return log(x)/log(2); }

// ----------------------------------
// TextureManager
// ----------------------------------
#include <unordered_map>
#include <unordered_set>

std::unordered_map<uint32_t, Texture> textures;
uint32_t textureIDcount = 10000;

struct TextureManager::Impl {
    uint32_t load(const char *path) {
        textures.emplace(textureIDcount, path);
        return textureIDcount++;
    }
    void bind(uint32_t texture_id) {
        auto it = textures.find(texture_id);
        it = textures.find(texture_id);
        if (it == textures.end()) {
            LOG(LOG_WARNING, "nonexistent texture bound with id %ud\n", texture_id);
            return;
        }
        it->second.bind();
    }
    void remove(uint32_t texture_id) {
        auto it = textures.find(texture_id);
        if (it == textures.end()) {
            LOG(LOG_WARNING, "nonexistent texture delete with id %ud\n", texture_id);
            return;
        }
        textures.erase(it);
    }
};

TextureManager::ImplPtr TextureManager::m_impl = nullptr;

bool TextureManager::startup(void) { m_impl = ImplPtr(new Impl); return true; }
void TextureManager::shutdown(void) { m_impl.reset(nullptr); }
uint32_t TextureManager::load(const char *path) { return m_impl->load(path); }
void TextureManager::bind(uint32_t textureID) { m_impl->bind(textureID); }
void TextureManager::remove(uint32_t textureID) { m_impl->remove(textureID); }

#include "./LuaBindings.h"
int L_LoadTexture(lua_State *L) {
    if (lua_gettop(L) != 1)
        luaL_error(L, "Bad LoadTexture() call");
    auto arg1 = lua_tostring(L, 1);
    auto texID = TextureManager::load(arg1);
    lua_pop(L, 1);
    lua_pushinteger(L, texID);
    return 1;
}
int L_BindTexture(lua_State *L) {
    if (lua_gettop(L) != 1)
        luaL_error(L, "Bad BindTexture() call");
    auto textureID = lua_tointeger(L, 1);
    TextureManager::bind(textureID);
    return 0;
}
