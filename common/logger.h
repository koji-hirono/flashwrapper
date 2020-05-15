#ifndef LOGGER_H__
#define LOGGER_H__

#include <stdio.h>
#include <stdarg.h>

#define logger_debug(...) logger_printf(__func__, __LINE__, __VA_ARGS__)
#define logger_dump(s,d,z)  logger_hexdump(__func__, __LINE__,(s),(d),(z))

#define logger_file "/home/paleman/myflashplayer.log"

extern FILE *logger_fp;
extern void logger_printf(const char *, int, const char *, ...);
extern void logger_hexdump(const char *, int, const char *, const void *,
		size_t);

#endif
