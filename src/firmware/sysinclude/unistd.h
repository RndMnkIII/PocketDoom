/* Minimal unistd.h wrapper - forwards to libc.h */
#ifndef _UNISTD_H
#define _UNISTD_H
#include "libc.h"

/* access() mode flags */
#ifndef R_OK
#define R_OK 4
#define W_OK 2
#define X_OK 1
#define F_OK 0
#endif

#endif
