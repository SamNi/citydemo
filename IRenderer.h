#ifndef IRENDERER_H_
#define IRENDERER_H_
#include "IEntity.h"

struct IEntity;

struct IRenderer {
public:
    virtual void visit(const IEntity* ent) const = 0;
};

#endif  // IRENDERER_H_