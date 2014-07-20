#ifndef _PARTICLESYSTEM_H_
#define _PARTICLESYSTEM_H_
#include "essentials.h"
#include "GL.h"

struct ParticleSystem {
    ParticleSystem(void);

    void Init(int64_t nParticles);
    void Step(void);
    void Draw(void);


    GLuint ssbo_points, vao;
    GLuint initProgramID, initShaderID, stepProgramID, stepShaderID;
    int nParticles;
};

#endif // ~_PARTICLESYSTEM_H_