#ifndef IENTITY_H_
#define IENTITY_H_
#include "IRenderer.h"

struct IRenderer;

struct IEntity {
    virtual void accept(const IRenderer* r) const = 0;
};

#endif  // IENTITY_H_