#ifndef FRONTEND_H_
#define FRONTEND_H_
#include "./essentials.h"
#include "GLM.h"
#include "IRenderer.h"

namespace Frontend {

bool Startup(int w, int h);
void Render(void);
void Shutdown(void);
const IRenderer& getRenderer(void);

}  // namespace Frontend

#endif  // FRONTEND_H_