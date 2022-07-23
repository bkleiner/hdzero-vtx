#ifndef __DEBUG_H_
#define __DEBUG_H_

#include <stdarg.h>

void _debugf(const char *fmt, ...);
void _verbosef(const char *fmt, ...);

void debug_monitor();
#define debug_prompt() debugf("\r\nDM568X>")

#ifdef _DEBUG_MODE
#define debugf(args...) _debugf(args)
#define verbosef(args...) _verbosef(args)
#else

#ifdef SDCC
#define debugf
#define verbosef
#else
// workaround keils warning C275: expression with possibly no effect
static void _nopf(const char *fmt, ...) { fmt = fmt; }
#define debugf _nopf
#define verbosef _nopf
#endif

#endif

#endif /* __DEBUG_H_ */