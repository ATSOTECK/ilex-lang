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
