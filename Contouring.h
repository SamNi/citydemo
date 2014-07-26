// Copyright [year] <Copyright Owner>
#ifndef _CONTOURING_H_
#define _CONTOURING_H_
#include "essentials.h"
#include "GLM.h"

struct Triangle {
    glm::vec3 points[3];
};

struct GridCell {
    glm::vec3 corners[8];
    Real val[8];
};

struct VoxGrid {
    VoxGrid(int w, int h, int d);
    ~VoxGrid(void);

    GridCell& GetCell(int i, int j, int k);
    void Clear(void);
    void Draw(void);

    int SizeInBytes(void) const;

    GridCell *cells;
    int width, height, depth;
    int nTriangles;

};
glm::vec3 VertInterp(Real isolevel, glm::vec3 p1, glm::vec3 p2, Real valp1, Real valp2);
int Contour(const GridCell& gc, Real isolevel, Triangle *tris);

#endif // ~_CONTOURING_H_