#include "Contouring.h"
#include "Tables.h"
#include "GL.h"
#include "GLM.h"

using glm::vec3;
using glm::sin;

const float ISOLEVEL =          0.0f;

vec3 VertInterp(Real isolevel, vec3 p1, vec3 p2, Real valp1, Real valp2) {
   Real mu;
   vec3 p;

   if (glm::abs(isolevel-valp1) < 0.1)
      return(p1);
   if (glm::abs(isolevel-valp2) < 0.1)
      return(p2);
   if (glm::abs(valp1-valp2) < 0.1)
      return(p1);

   mu = (isolevel - valp1) / (valp2 - valp1);
   p = p1 + mu*(p2 - p1);
   return p;
}

int Contour(const GridCell& gc, Real isolevel, Triangle *tris) {
    uint8_t cubeindex = 0x00;
    int i, ntriang;
    vec3 verts[12];

    if (gc.val[0] < isolevel) cubeindex |= 1;
    if (gc.val[1] < isolevel) cubeindex |= 2;
    if (gc.val[2] < isolevel) cubeindex |= 4;
    if (gc.val[3] < isolevel) cubeindex |= 8;
    if (gc.val[4] < isolevel) cubeindex |= 16;
    if (gc.val[5] < isolevel) cubeindex |= 32;
    if (gc.val[6] < isolevel) cubeindex |= 64;
    if (gc.val[7] < isolevel) cubeindex |= 128;

    if (0 == edgeTable[cubeindex])
        return 0;

   if (edgeTable[cubeindex] & 1)
      verts[0] =
         VertInterp(isolevel,gc.corners[0],gc.corners[1],gc.val[0],gc.val[1]);
   if (edgeTable[cubeindex] & 2)
      verts[1] =
         VertInterp(isolevel,gc.corners[1],gc.corners[2],gc.val[1],gc.val[2]);
   if (edgeTable[cubeindex] & 4)
      verts[2] =
         VertInterp(isolevel,gc.corners[2],gc.corners[3],gc.val[2],gc.val[3]);
   if (edgeTable[cubeindex] & 8)
      verts[3] =
         VertInterp(isolevel,gc.corners[3],gc.corners[0],gc.val[3],gc.val[0]);
   if (edgeTable[cubeindex] & 16)
      verts[4] =
         VertInterp(isolevel,gc.corners[4],gc.corners[5],gc.val[4],gc.val[5]);
   if (edgeTable[cubeindex] & 32)
      verts[5] =
         VertInterp(isolevel,gc.corners[5],gc.corners[6],gc.val[5],gc.val[6]);
   if (edgeTable[cubeindex] & 64)
      verts[6] =
         VertInterp(isolevel,gc.corners[6],gc.corners[7],gc.val[6],gc.val[7]);
   if (edgeTable[cubeindex] & 128)
      verts[7] =
         VertInterp(isolevel,gc.corners[7],gc.corners[4],gc.val[7],gc.val[4]);
   if (edgeTable[cubeindex] & 256)
      verts[8] =
         VertInterp(isolevel,gc.corners[0],gc.corners[4],gc.val[0],gc.val[4]);
   if (edgeTable[cubeindex] & 512)
      verts[9] =
         VertInterp(isolevel,gc.corners[1],gc.corners[5],gc.val[1],gc.val[5]);
   if (edgeTable[cubeindex] & 1024)
      verts[10] =
         VertInterp(isolevel,gc.corners[2],gc.corners[6],gc.val[2],gc.val[6]);
   if (edgeTable[cubeindex] & 2048)
      verts[11] =
         VertInterp(isolevel,gc.corners[3],gc.corners[7],gc.val[3],gc.val[7]);

   ntriang = 0;
   for (i = 0;triTable[cubeindex][i] != -1;i += 3) {
       tris[ntriang].points[0] = verts[triTable[cubeindex][i+0]];
       tris[ntriang].points[1] = verts[triTable[cubeindex][i+1]];
       tris[ntriang].points[2] = verts[triTable[cubeindex][i+2]];
       ntriang++;
   }

   return ntriang;
}

VoxGrid::VoxGrid(int w, int h, int d) :
    width(w), height(h), depth(d), cells(nullptr), nTriangles(0)
{
    assert((w>0) && (h>0) && (d>0));

    cells = new GridCell[width*height*depth];
    Clear();
}

VoxGrid::~VoxGrid(void) {
    delete[] cells;
}

GridCell& VoxGrid::GetCell(int i, int j, int k) {
    int addr;
    
    addr = i*height*depth + depth*j + k;
    return cells[addr];
}

void VoxGrid::Clear(void) {
    int i, j, k;
    
    for (i = 0;i < width;++i) {
        for (j = 0;j < height;++j) {
            for (k = 0;k < depth;++k) {
                int x, y, z;

                x = i - (width/2);
                y = j - (height/2);
                z = k - (depth/2);

                // http://paulbourke.net/geometry/polygonise/polygonise1.gif
                /*
                GetCell(i,j,k).corners[0] = vec3((float)x+0, (float)y+0, (float)z+0);
                GetCell(i,j,k).corners[1] = vec3((float)x+1, (float)y+0, (float)z+0);
                GetCell(i,j,k).corners[2] = vec3((float)x+0, (float)y+0, (float)z+1);
                GetCell(i,j,k).corners[3] = vec3((float)x+1, (float)y+0, (float)z+1);
                GetCell(i,j,k).corners[4] = vec3((float)x+0, (float)y+1, (float)z+0);
                GetCell(i,j,k).corners[5] = vec3((float)x+1, (float)y+1, (float)z+0);
                GetCell(i,j,k).corners[6] = vec3((float)x+0, (float)y+1, (float)z+1);
                GetCell(i,j,k).corners[7] = vec3((float)x+1, (float)y+1, (float)z+1);
                */
                GetCell(i,j,k).corners[0] = vec3((float)x+0, (float)y+0, (float)z+1);
                GetCell(i,j,k).corners[1] = vec3((float)x+1, (float)y+0, (float)z+1);
                GetCell(i,j,k).corners[2] = vec3((float)x+1, (float)y+0, (float)z+0);
                GetCell(i,j,k).corners[3] = vec3((float)x+0, (float)y+0, (float)z+0);
                GetCell(i,j,k).corners[4] = vec3((float)x+0, (float)y+1, (float)z+1);
                GetCell(i,j,k).corners[5] = vec3((float)x+1, (float)y+1, (float)z+1);
                GetCell(i,j,k).corners[6] = vec3((float)x+1, (float)y+1, (float)z+0);
                GetCell(i,j,k).corners[7] = vec3((float)x+0, (float)y+1, (float)z+0);
                int n;
                for (n = 0;n < 8;++n) {
                    vec3 corner = GetCell(i,j,k).corners[n];
                    float mag = glm::sqrt( corner.x*corner.x + corner.y*corner.y + corner.z*corner.z );
                    float val = glm::sin(0.08*mag);
                    GetCell(i,j,k).val[n] = val;
                }
            }
        }
    }
    //memset(cells, 0, sizeof(GridCell)*width*height*depth);
}

void VoxGrid::Draw(void) {
    static bool calledBefore = false;
    static GLuint vbo_points;
    static GLuint vao;

    if (!calledBefore) {
        GLfloat *points;
        Triangle tris[5];
        int i, j, k, n, c, x;

        points = new GLfloat[15*1024*1024];
        c = 0;
        for (i = 0;i < width;++i) {
            for (j = 0;j < height;++j) {
                for (k = 0;k < depth;++k) {
                    n = Contour(GetCell(i,j,k), ISOLEVEL, tris);
                    assert(n <= 5);
                    for (x = 0;x < n;++x) {
                        points[c++] = tris[x].points[0].x;
                        points[c++] = tris[x].points[0].y;
                        points[c++] = tris[x].points[0].z;
                        points[c++] = tris[x].points[1].x;
                        points[c++] = tris[x].points[1].y;
                        points[c++] = tris[x].points[1].z;
                        points[c++] = tris[x].points[2].x;
                        points[c++] = tris[x].points[2].y; 
                        points[c++] = tris[x].points[2].z;
                    }
                    nTriangles += n;
                }
            }
        }


        glGenBuffers(1, &vbo_points);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_points);
        glBufferData(GL_ARRAY_BUFFER, 3*3*nTriangles*sizeof(GLfloat), points, GL_STATIC_DRAW);

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_points);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        glEnableVertexAttribArray(0);

        calledBefore = true;
        delete[] points;
    }
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3*3*nTriangles);
    checkGL();
}

int VoxGrid::SizeInBytes(void) const {
    return width*height*depth*sizeof(GridCell) + sizeof(VoxGrid);
}