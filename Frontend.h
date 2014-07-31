#ifndef FRONTEND_H_
#define FRONTEND_H_
#include "./essentials.h"
#include "GLM.h"

namespace Frontend {

bool Startup(int w, int h);
void Render(void);
void Shutdown(void);


}  // namespace Frontend

#endif  // FRONTEND_H_