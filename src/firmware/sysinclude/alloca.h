/* Minimal alloca.h - uses GCC builtin */
#ifndef _ALLOCA_H
#define _ALLOCA_H

#define alloca(size) __builtin_alloca(size)

#endif
