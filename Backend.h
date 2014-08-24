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

// typedefs
typedef glm::u8vec3 RGBPixel;
typedef glm::u8vec4 RGBAPixel;
typedef glm::u16vec2 TexCoord;
typedef uint32_t PackedNormal;

// forward decls
struct Framebuffer;
struct Image;
struct PerfCounters;
struct Resource;
struct SurfaceTriangles;
struct Texture;
struct TextureManager;

// enums
enum ImageFormat;

// primary interface: try to keep this near the top
class Backend {
public:
    static bool     startup(int w, int h);
    static void     begin_frame(void);
    static void     end_frame(void);
    static void     shutdown(void);

    static void     resize(int w, int h);
    static RGBPixel*     get_screenshot(void);
    static RGBPixel*     read_screenshot(const char *path);
    static void     write_screenshot(void);
    static bool     write_screenshot(const char *fname);
    static void     add_random_tris(void);

    static uint32_t add_surface_triangles(std::shared_ptr<SurfaceTriangles> st);
    static void     draw_surface_triangles(uint32_t handle);
    static void     set_modelview(const glm::mat4x4& m);
    static void     set_projection(const glm::mat4x4& m);

    static void     set_clear_color(const RGBPixel& color);
    static void     set_clear_color(const RGBAPixel& color);
    static void     enable_depth_testing(void);
    static void     disable_depth_testing(void);
    static void     enable_blending(void);
    static void     enable_additive_blending(void);
    static void     disable_blending(void);
    static void     show_hud(bool b);

    static const PerfCounters& get_performance_count(void);

private:
    struct Impl;
    static std::unique_ptr<Impl> mImpl;
};

struct SurfaceTriangles {
    SurfaceTriangles(void);
    SurfaceTriangles(const uint32_t nv, const uint32_t nidx);
    ~SurfaceTriangles(void);
    uint32_t GetSize(void) const;
    uint32_t nVertices;
    glm::vec3 *vertices;
    RGBAPixel *colors;
    TexCoord *texture_coordinates;
    PackedNormal *normals;

    uint32_t nIndices;
    uint32_t *indices;
};

// Per-frame performance stats
struct PerfCounters {
    uint32_t        n_triangles_drawn;
};

// Various GL specs
struct Specs {
    // Try to keep in alphabetical order
    int             nExtensions;
    int             nMaxCombinedTextureImageUnits;
    int             nMaxDrawBuffers;
    int             nMaxElementsIndices;
    int             nMaxElementsVertices;
    int             nMaxFragmentUniformBlocks;
    int             nMaxGeometryUniformBlocks;
    int             nMaxTextureImageUnits;
    int             nMaxUniformBufferBindings;
    int             nMaxVertexAttribs;
    int             nMaxVertexUniformBlocks;

    const unsigned char*  renderer;
    const unsigned char*  vendor;
    const unsigned char*  version;
    std::vector<const unsigned char*> extensions;
};

// Texture.cpp
struct TextureManager { 
    static bool                     startup(void);
    static void                     shutdown(void);
    static uint32_t                 load(const char *path);
    static void                     bind(uint32_t textureID);
    static void                     remove(uint32_t textureID);
private:
    struct Impl;
    typedef std::unique_ptr<Impl> ImplPtr;
    static ImplPtr m_impl;
};

enum ImageFormat {
    RGB_,
    RGBALPHA,
    GRAYSCALE
};

struct Image {
    int w, h;
    uint8_t *pixels;
    ImageFormat fmt;
};

// Image : API-independent
// Texture : Everything needed to function with OpenGL + the associated image
struct Texture {
    Texture(void);
    Texture(int w, int h);
    Texture(const char *fname, bool filtered = true, bool mipmapped = true);
    ~Texture(void);

    void bind(void) const;
    void refresh(void);

    uint8_t *get_pixels(void) const;
    uint32_t get_texture_id(void) const;
    const char *get_name(void) const;
    size_t get_size_in_bytes(void) const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

#endif // ~_BACKEND_H_