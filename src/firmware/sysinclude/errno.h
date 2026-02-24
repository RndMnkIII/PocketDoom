/* Minimal errno.h wrapper - forwards to libc.h */
#ifndef _ERRNO_H
#define _ERRNO_H
#include "libc.h"

#ifndef ENOMEM
#define ENOMEM 12
#endif
#ifndef EACCES
#define EACCES 13
#endif

#endif
