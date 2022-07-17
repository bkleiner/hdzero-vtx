#ifndef __PRINT_H__
#define __PRINT_H__

#include <stdarg.h>

#ifdef _DEBUG_MODE
void debugf(const char *fmt, ...);
#endif
#endif