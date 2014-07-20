// http://www.cs.otago.ac.nz/homepages/andrew/papers/g2.pdf
// http://paulbourke.net/geometry/implicitsurf/
// TODO: Debug simulation, add rendering
#include "Cloud.h"

struct Cell {
    bool act, hum, cld;
};

Cloud::Cloud(int16_t w, int16_t h, int16_t d) : width(w), height(h), depth(d) {
    assert(w && h && d);
    m_cells = new Cell[w*h*d];
    assert(m_cells);
    //memset(m_cells, NULL, w*h*d*sizeof(Cell));
    for(int i = 0;i<(w*h*d);++i) {
        m_cells[i].hum = (bool)(rand()%2);
        m_cells[i].cld = (bool)(rand()%2);
        m_cells[i].act = (bool)(rand()%2);
    }
}
Cloud::~Cloud(void) {
    if(m_cells)
        delete[] m_cells;
}

Cell Cloud::GetCell(int16_t x, int16_t y, int16_t z) const {
    if(within(x,y,z))
        return m_cells[coord2addr(x,y,z)];
    Cell c = { false, false, false };
    return c;
}

void Cloud::SetCell(int16_t x, int16_t y, int16_t z, const Cell& val) {
    assert(within(x,y,z));
    int addr = coord2addr(x,y,z);
    m_cells[addr] = val;
}

inline bool Cloud::within(int16_t x, int16_t y, int16_t z) const { 
    return (0<=x) && (x<width) && (0<=y) && (y<height) && (0<=z) && (z<depth); 
}

bool Cloud::f_act(int16_t x, int16_t y, int16_t z) const {
    bool ret = GetCell(x+1,y,z).act ||
        GetCell(x, y+1, z).act ||
        GetCell(x, y, z+1).act ||
        GetCell(x-1,y, z).act ||
        GetCell(x, y-1, z).act ||
        GetCell(x, y, z-1).act ||
        GetCell(x-2, y, z).act ||
        GetCell(x+2, y, z).act ||
        GetCell(x, y-2, z).act ||
        GetCell(x, y+2, z).act ||
        GetCell(x, y, z-2).act;
    return ret;
}

void Cloud::Step(void) {
    Cloud *next = new Cloud(width, height, depth);

    for(int16_t i=0;i<width;++i) {
        for(int16_t j=0;j<height;++j) {
            for(int16_t k=0;k<depth;++k) {
                // http://nis-ei.eng.hokudai.ac.jp/~doba/papers/sig00_cloud.pdf
                Cell c; 
                c.hum = GetCell(i,j,k).hum && !GetCell(i,j,k).hum;
                c.cld = GetCell(i,j,k).cld || GetCell(i,j,k).act;
                c.act = !GetCell(i,j,k).act && GetCell(i,j,k).hum && f_act(i,j,k);

                next->SetCell(i,j,k,c);
            }
        }
    }

    memcpy(this->m_cells, next->m_cells, width*height*depth*sizeof(Cell));
    delete next;
}

void Cloud::Display(void) const {
    fprintf(stderr, "(hum cld act)\n");
    fprintf(stderr,"--- begin ---\n");
    for(int16_t i=0;i<width;++i) {
        for(int16_t j=0;j<height;++j) {
            fprintf(stderr,"(");
            for(int16_t k=0;k<depth;++k) {
                Cell c = GetCell(i,j,k);
                fprintf(stderr,"(%d %d %d),", c.hum, c.cld, c.act);
            }
            fprintf(stderr,")\n");
        }
        fprintf(stderr,",\n");
    }
    fprintf(stderr,"---- end ----\n");
}

inline int Cloud::coord2addr(int16_t x, int16_t y, int16_t z) const {
    assert(within(x,y,z));
    return x*height*depth + depth*y + z;
}