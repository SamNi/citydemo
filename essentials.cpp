// Copyright [year] <Copyright Owner>
#include "essentials.h"
#include "GL.h"
#include <physfs/physfs.h>

// caller's responsibility to free the returned buf
char *readFile(const char *fname) {
    PHYSFS_file *fin = nullptr;
    PHYSFS_sint64 fileLen;
    char *buf = nullptr;
    fin = PHYSFS_openRead(fname);

    if (nullptr == fin)
        return nullptr;

    fileLen = PHYSFS_fileLength(fin);
    buf = new char[fileLen+1];
    PHYSFS_read(fin, buf, 1, fileLen);
    buf[fileLen] = '\0';            // ~physfs
    PHYSFS_close(fin);
    return buf;
}

void _log(LogLevel l, const char *srcFilePath, int lineNo, const char *funcName, const char *msg, ... ) {
#ifdef _TEST_BUILD
    static LogLevel log_threshold = LOG_VERBOSE;
#else
    static LogLevel log_threshold = LOG_TRACE;
#endif
    static const char *levelMnemonic[] = { "!", "W", "I", "V", "T" };
    static char dateBuf[60];
    static char filenameBuf[256];
    static char extensionBuf[256];

    static time_t rightNow;
    static tm timeStruct;
    rightNow = time(NULL);
    timeStruct = (*localtime(&rightNow));
    strftime(dateBuf, 60, "%Y-%m-%d %H:%M:%S", &timeStruct);

    va_list args;
    char msgBuf[4096];
    va_start(args, msg);
    vsprintf(msgBuf, msg, args);
    va_end(args);

    _splitpath(srcFilePath, nullptr, nullptr, filenameBuf, extensionBuf);
    if (l <= log_threshold)
        printf("%s;%s%s:%d;%s;%s; %s\n", levelMnemonic[l], filenameBuf, extensionBuf, lineNo, funcName, dateBuf, msgBuf);

    fflush(stdout);
}

// png images are stored in the opposite direction than OpenGL expects
void imgflip(int w, int h, int nComponents, uint8_t *pixels) {
    int rowSize = sizeof(uint8_t)*w*nComponents;
    uint8_t *tmp = new uint8_t[rowSize];
    int i;
    for (i = 0;i < (h>>1);++i) {
        memcpy(tmp, &pixels[i*rowSize], rowSize);
        memcpy(&pixels[i*rowSize], &pixels[(h - i - 1)*rowSize], rowSize);
        memcpy(&pixels[(h - i - 1)*rowSize], tmp, rowSize);
    }
}

void checkGL(void) {
#ifdef _DEBUG
    GLuint err = glGetError();
    if (GL_NO_ERROR!=err) {
        LOG(LOG_CRITICAL, "%s\n", gluErrorString(err));
#ifndef _TEST_BUILD
        exit(EXIT_FAILURE);
#endif
    }
#endif
}

void APIENTRY debugproc(GLenum source, GLenum type, GLuint id, GLenum severity,
               GLsizei length, const GLchar *incoming, void *userParam) {
    static std::multiset<GLuint> past_errs;
    static const uint8_t MAX_ERR_OCCURRENCES = 12;
    static FILE *fout = stderr;
    static char outMsg[4096] = "<intentionally left blank>";
    static const char *srcMsg = nullptr;
    static const char *typeMsg = nullptr;
    static const char *severityMsg = nullptr;

    if (past_errs.count(id) > MAX_ERR_OCCURRENCES)
        return;

    switch (source) {
    case GL_DEBUG_SOURCE_API:
        srcMsg = "API";
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        srcMsg = "Window System";
        break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        srcMsg = "Shader Compiler";
        break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        srcMsg = "3rd Party";
        break;
    case GL_DEBUG_SOURCE_APPLICATION:
        srcMsg = "Application";
        break;
    case GL_DEBUG_SOURCE_OTHER :
        srcMsg = "Other";
        break;
    default:
        srcMsg = "???";
        break;
    }
    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        typeMsg = "Error";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        typeMsg = "Deprecated behavior";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        typeMsg = "Undefined behavior";
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        typeMsg = "Portability";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        typeMsg = "Performance";
        break;
    case GL_DEBUG_TYPE_MARKER:
        typeMsg = "Marker";
        break;
    case GL_DEBUG_TYPE_PUSH_GROUP:
        typeMsg = "Push group";
        break;
    case GL_DEBUG_TYPE_POP_GROUP:
        typeMsg = "Pop group";
        break;
    case GL_DEBUG_TYPE_OTHER:
        typeMsg = "other";
        break;
    default:
        typeMsg = "???";
        break;
    }
    switch (severity) {
    case GL_DEBUG_SEVERITY_LOW:
        severityMsg = "Minor";
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        severityMsg = "Significant";
        break;
    case GL_DEBUG_SEVERITY_HIGH:
        severityMsg = "Critical";
        break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        severityMsg = "Info";
        break;
    default:
        severityMsg = "???";
        break;
    }
    sprintf(outMsg, "(%s, %s from %s) #%u\n%s\n", severityMsg, typeMsg, srcMsg, id, incoming);
    fprintf(fout, outMsg);

    past_errs.emplace(id);
    if (past_errs.count(id) == MAX_ERR_OCCURRENCES) {
        fprintf(fout, "This is the last time I will report message %u.\n", id);
    }
}

