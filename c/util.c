//
// Created by Skyler on 05/21/22.
//

#include "util.h"

#ifdef I_WIN
static void fseterr(FILE *fp) {
    struct file {
        unsigned char *_ptr;
        unsigned char *_base;
        int _cnt;
        int _flag;
        int _file;
        int _charbuf;
        int _bufsiz;
    };
#define IOERR_ 0x10
    ((struct file *)fp)->_flag |= IOERR_;
}

ssize_t getline(char **restrict line, size_t *restrict n, FILE *restrict file) {
    if (line == NULL || n == NULL || file == NULL) {
        return -1;
    }
    
    if (feof(file) || ferror(file)) {
        return -1;
    }
    
    if (*line == NULL) {
        *n = 128;
        *line = (char*)malloc(sizeof(char) * *n);
        if (*line == NULL) {
            fseterr(file);
            return -1;
        }
    }
    
    ssize_t len = 0;
    int c = EOF;
    while (c != '\n') {
        c = fgetc(file);
        if (c == EOF) {
            break;
        }
        
        if (len >= *n - 1) {
            *n += 128;
            *line = (char*)realloc(*line, sizeof(char) * *n);
            if (*line == NULL) {
                fseterr(file);
                return -1;
            }
        }
        (*line)[len++] = (char)c;
    }
    
    if (c == EOF && len == 0) {
        return -1;
    }
    
    (*line)[len] = '\0';
    *n = len;
    
    return len;
}
#endif

bool resolvePath(const char *dir, const char *path, char *ret) {
    char buf[I_MAX_PATH];
    if (*path == PATH_SEPARATOR) {
        snprintf(buf, I_MAX_PATH, "%s", path);
    } else {
        snprintf(buf, I_MAX_PATH, "%s%c%s", dir, PATH_SEPARATOR, path);
    }

#ifdef I_WIN
    _fullpath(ret, buf, I_MAX_PATH);
#else
    if (realpath(buf, ret) == NULL) {
        return false;
    }
#endif
    
    return true;
}

ObjString *dirName(VM *vm, const char *path, size_t len) {
    if (len == 0) {
        return copyString(vm, ".", 1);
    }
    
    char *sep = (char*)path + len;
    while (sep != path) {
        if (!IS_SEP(*sep)) {
            break;
        }
        --sep;
    }
    
    while (sep != path) {
        if (IS_SEP(*sep)) {
            break;
        }
        --sep;
    }
    
    while (sep != path) {
        if (!IS_SEP(*sep)) {
            break;
        }
        --sep;
    }
    
    if (sep == path && !IS_SEP(*sep)) {
        return copyString(vm, ".", 1);
    }
    
    len = sep - path + 1;
    return copyString(vm, path, (int)len);
}

ObjString *getDir(VM *vm, const char *source) {
    char res[I_MAX_PATH];
    if (!resolvePath(".", source, res)) {
        runtimeError(vm, "Unable to resolve path '%s'.", source);
        exit(1);
    }
    
    return dirName(vm, res, strlen(res));
}

char* readFile(const char *path) {
#ifdef I_WIN
    FILE* file;
    errno_t err = fopen_s(&file, path, "rb");
#else
    FILE* file = fopen(path, "rb");
    errno_t = file == NULL ? -1 : 0;
#endif
    if (err != 0) {
        fprintf(stderr, "Could not open file '%s'.\n", path);
        exit(74);
    }
    
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);
    
    char *buffer = (char *)malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read '%s'.\n", path);
        exit(74);
    }
    
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file '%s'.\n", path);
        exit(74);
    }
    
    buffer[bytesRead] = '\0';
    
    fclose(file);
    return buffer;
}
