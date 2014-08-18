// Copyright [year] <Copyright Owner>
#include "./ParticleSystem.h"






/*
#include "ParticleSystem.h"
#include "GLM.h"

ParticleSystem::ParticleSystem(void) {
    // deliberately left blank
}

void ParticleSystem::Init(int64_t n) {
    GLint result;
    int i;

    assert(n>=0);
    nParticles = n;

    checkGL();

    // SSBO
    glGenBuffers(1, &ssbo_points);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_points);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 3*nParticles*sizeof(GLfloat), nullptr, GL_DYNAMIC_COPY);
    checkGL();

    // VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, ssbo_points);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);
    checkGL();

    LoadComputeShader("InitParticle.compute", initProgramID, initShaderID);
    LoadComputeShader("StepParticle.compute", stepProgramID, stepShaderID);

    glUseProgram(initProgramID);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_points);
    glDispatchCompute( (GLuint)glm::ceil(nParticles/64.0f), 1, 1);
    checkGL();
}

void ParticleSystem::LoadComputeShader(const char *fname, GLuint& progID, GLuint& shadID) {
    GLint result;
    const char *shadSource = readFile(fname);

    // Compute shaders
    progID = glCreateProgram();
    shadID = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shadID, 1, &shadSource, nullptr);
    glCompileShader(shadID);
    checkGL();
    glGetShaderiv(shadID, GL_COMPILE_STATUS, &result);

    if (!result) {
        static char tmp[GL_INFO_LOG_LENGTH];
        GLsizei len;

        glGetShaderInfoLog(shadID, GL_INFO_LOG_LENGTH-1, &len, tmp);
        tmp[len]='\0';
        LOG(LOG_CRITICAL, "%s\n", tmp);
        exit(EXIT_FAILURE);
    }

    glAttachShader(progID, shadID);
    glLinkProgram(progID);
    glGetProgramiv(progID, GL_LINK_STATUS, &result);
    checkGL();
    if (!result) {
        static char tmp[GL_INFO_LOG_LENGTH];
        GLsizei len;

        glGetProgramInfoLog(progID, GL_INFO_LOG_LENGTH-1, &len, tmp);
        tmp[len]='\0';
        LOG(LOG_CRITICAL, "%s\n", tmp);
        exit(EXIT_FAILURE);
    }

    delete[] shadSource;
}
void ParticleSystem::Step(void) {
    glUseProgram(stepProgramID);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_points);
    glDispatchCompute((GLuint)glm::ceil(nParticles/32.0f), 1, 1);
}

// make sure the vert/frag programs are in use before calling this (client responsibility)
void ParticleSystem::Draw(void) {
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, ssbo_points);
    glDrawArrays(GL_TRIANGLES, 0, nParticles);
}
*/
/*
#include "ParticleSystem.h"

void Particle::accept(const IRenderer* r) const {
    r->visit(this);
}

PSystem::PSystem(int n) : nParticles(n) {
    elem = new Particle[nParticles];
    for (int i = 0;i < nParticles; ++i) {
        elem[i].pos = glm::vec3( uniform(-1.0f, 1.0f), uniform(-1.0f, 1.0f), uniform(-1.0f, 1.0f));
        elem[i].timeLeft = uniform(7.0f, 8.0f);
    }
}

PSystem::~PSystem(void) {
    delete[] elem;
}

void PSystem::accept(const IRenderer* r) const {
    for (int i = 0;i < nParticles;++i)
        r->visit(&elem[i]);
}
*/
