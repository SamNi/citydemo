#include "Backend_local.h"

SurfaceTriangles::SurfaceTriangles(void) { 
    wipe_memory(this, sizeof(SurfaceTriangles)); 
}
SurfaceTriangles::SurfaceTriangles(const uint32_t nv, const uint32_t nidx) {
    assert(nv && nidx);
    wipe_memory(this, sizeof(SurfaceTriangles));
    vertices = new glm::vec3[nv];
    colors = new RGBAPixel[nv];
    texture_coordinates = new TexCoord[nv];
    normals = new PackedNormal[nv];
    indices = new GLuint[nidx];
    nVertices = nv;
    nIndices = nidx;
}
SurfaceTriangles::~SurfaceTriangles(void) {
    delete[] vertices;
    delete[] colors;
    delete[] texture_coordinates;
    delete[] normals;
    delete[] indices;
}
uint32_t SurfaceTriangles::GetSize(void) const {
    uint32_t ret = 0;
    if (nullptr != vertices)
        ret += sizeof(glm::vec4)*nVertices;
    if (nullptr != colors)
        ret += sizeof(glm::vec4)*nVertices;
    if (nullptr != texture_coordinates)
        ret += sizeof(glm::vec2)*nVertices;
    if (nullptr != normals)
        ret += sizeof(GLuint)*nVertices;
    if (nullptr != indices)
        ret += sizeof(GLuint)*nIndices;
    return ret;
}
