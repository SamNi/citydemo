#ifndef _CLOUD_H_
#define _CLOUD_H_
#include "essentials.h"
#include "GL.H"

struct Cell;
struct Coord {
    int16_t x, y, z;
};

struct Cloud {
    Cloud(int16_t w, int16_t h, int16_t d);
    ~Cloud();

    void Step(void);
    void Display(void) const;

    inline int coord2addr(int16_t x, int16_t y, int16_t z) const;

private:
    inline Cell GetCell(int16_t x, int16_t y, int16_t z) const;
    inline void SetCell(int16_t x, int16_t y, int16_t z, const Cell& val);
    inline bool within(int16_t x, int16_t y, int16_t z) const;
    bool f_act(int16_t x, int16_t y, int16_t z) const;

    int16_t width, height, depth;
    Cell *m_cells;
};
#endif // ~_CLOUD_H_