#include "ParticleSystem.h"

static const char *initShaderSource =
    "#version 440 core\n"\
    "layout(local_size_x = 64) in;\n"\
    "struct pos { float x, y, z; };"\
    "layout(std430, binding = 4) buffer shader_data { pos points[]; };\n"\
    "\n"\
    "float rand(float seed) { return fract(sin(dot(gl_GlobalInvocationID.xy+vec2(seed,seed), vec2(12.9898,78.233))) * 43758.5453); }"\
    "float unif(float seed, float a, float b) {return a + (b-a)*rand(seed);}\n"\
    "void main() { \n"\
    "   points[gl_GlobalInvocationID.x].x = unif(15, -1.0f, 1.0f);"\
    "   points[gl_GlobalInvocationID.x].y = unif(16, -1.0f, 1.0f);"\
    "   points[gl_GlobalInvocationID.x].z = unif(17, -0.1f, 0.1f);"\
    "}\n"\
    ;

static const char *stepShaderSource = "";


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

    // Compute shaders
    initProgramID = glCreateProgram();
    initShaderID = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(initShaderID, 1, &initShaderSource, nullptr);
    glCompileShader(initShaderID);
    checkGL();
    glGetShaderiv(initShaderID, GL_COMPILE_STATUS, &result);

    if (!result) {
        static char tmp[GL_INFO_LOG_LENGTH];
        GLsizei len;

        glGetShaderInfoLog(initShaderID, GL_INFO_LOG_LENGTH-1, &len, tmp);
        tmp[len]='\0';
        fprintf(stderr, "%s\n", tmp);
        exit(EXIT_FAILURE);
    }

    glAttachShader(initProgramID, initShaderID);
    glLinkProgram(initProgramID);
    glGetProgramiv(initProgramID, GL_LINK_STATUS, &result);
    checkGL();
    if (!result) {
        static char tmp[GL_INFO_LOG_LENGTH];
        GLsizei len;

        glGetProgramInfoLog(initProgramID, GL_INFO_LOG_LENGTH-1, &len, tmp);
        tmp[len]='\0';
        fprintf(stderr, "%s\n", tmp);
        exit(EXIT_FAILURE);
    }
    glUseProgram(initProgramID);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_points);
    glDispatchCompute( (GLuint)glm::ceil(nParticles/64.0f), 1, 1);

    // VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, ssbo_points);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);
    checkGL();
}

void ParticleSystem::Step(void) {
}

void ParticleSystem::Draw(void) {
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, ssbo_points);
    glDrawArrays(GL_TRIANGLES, 0, nParticles);
    checkGL();
}