/* Minimal sys/stat.h */
#ifndef _SYS_STAT_H
#define _SYS_STAT_H

#include "libc.h"

struct stat {
    unsigned short st_dev;
    unsigned short st_ino;
    unsigned int   st_mode;
    unsigned short st_nlink;
    unsigned short st_uid;
    unsigned short st_gid;
    unsigned short st_rdev;
    off_t          st_size;
    long           st_atime;
    long           st_mtime;
    long           st_ctime;
};

int fstat(int fd, struct stat *buf);
int stat(const char *path, struct stat *buf);

#endif
