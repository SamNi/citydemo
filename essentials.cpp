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
    static LogLevel log_threshold = LOG_TRACE;
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
        exit(EXIT_FAILURE);
    }
#endif
}
