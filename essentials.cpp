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
/*
char *readFile(const char *fname) {
    FILE *fin;
    size_t count;
    char *ret;

    fin = fopen(fname, "rb");
    if (!fin)
        return NULL;
    fseek(fin, 0, SEEK_END);
    count = ftell(fin);
    rewind(fin);
    ret = new char[count+1];
    if(count != fread(ret, sizeof(char), count, fin)) {
        fprintf(stderr, "Error reading %s\n", fname);
        delete[] ret;
        return NULL;
    }
    ret[count]='\0';
    return ret;
}
*/

void checkGL(void) {
#ifdef _DEBUG
    GLuint err = glGetError();
    if (GL_NO_ERROR!=err) {
        fprintf(stderr, "%s\n", gluErrorString(err));
        exit(EXIT_FAILURE);
    }
#endif
}
