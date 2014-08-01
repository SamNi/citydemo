// Copyright [year] <Copyright Owner>
#ifndef _PARTICLESYSTEM_H_
#define _PARTICLESYSTEM_H_
/*
#include "essentials.h"
#include "GL.h"

struct ParticleSystem {
    ParticleSystem(void);

    void Init(int64_t nParticles);
    void Step(void);
    void Draw(void);

private:
    void LoadComputeShader(const char *fname, GLuint& progID, GLuint& shadID);


    GLuint ssbo_points, vao;
    GLuint initProgramID, initShaderID, stepProgramID, stepShaderID;
    int64_t nParticles;
    
};
*/
#include "essentials.h"
#include "GLM.h"
#include "./IEntity.h"

struct Particle : public IEntity {
    glm::vec3 pos;
    float timeLeft;

    virtual void accept(const IRenderer* r) const;
};

struct PSystem : public IEntity {
    PSystem(int n);
    ~PSystem(void);

    virtual void accept(const IRenderer* r) const;
    Particle *elem;
    int nParticles;
};

#endif // ~_PARTICLESYSTEM_H_