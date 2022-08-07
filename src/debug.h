#ifndef __DEBUG_H_
#define __DEBUG_H_

#include <stdarg.h>

#include "config.h"
#include "stdint.h"

void _debugf(const char *fmt, ...);
void _verbosef(const char *fmt, ...);

void _debug_hex(uint8_t *data, uint16_t size);

#ifdef DEBUG_MODE
#define debugf(args...) _debugf(args)
#define debug_hex(data, size) _debug_hex(data, size)
#define verbosef(args...) _verbosef(args)
#else

#ifdef SDCC
#define debugf(args...)
#define debug_hex(data, size)
#define verbosef(args...)
#else
// workaround keils warning C275: expression with possibly no effect
static void _nopf(const char *fmt, ...) { fmt = fmt; }
#define debugf _nopf
#define debug_hex _nopf
#define verbosef _nopf
#endif

#endif

#endif /* __DEBUG_H_ */
