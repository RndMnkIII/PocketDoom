/* Minimal fcntl.h */
#ifndef _FCNTL_H
#define _FCNTL_H
#include "libc.h"

#ifndef O_RDONLY
#define O_RDONLY 0
#endif
#ifndef O_WRONLY
#define O_WRONLY 1
#endif
#ifndef O_RDWR
#define O_RDWR   2
#endif
#ifndef O_CREAT
#define O_CREAT  0x0040
#endif
#ifndef O_TRUNC
#define O_TRUNC  0x0200
#endif
#ifndef O_APPEND
#define O_APPEND 0x0400
#endif
#ifndef O_BINARY
#define O_BINARY 0
#endif

#endif
