#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

#include "logger.h"


FILE *logger_fp = NULL;


void
logger_printf(const char *func, int line, const char *fmt, ...)
{
	char buf[16];
	struct timespec ts;
	struct tm tm;
	int error;
	va_list ap;

	error = errno;

	if (logger_fp == NULL) {
		logger_fp = fopen(logger_file, "a");
		if (logger_fp == NULL) {
			errno = error;
			return;
		}
		setvbuf(logger_fp, NULL, _IONBF, 0);
	}

	clock_gettime(CLOCK_REALTIME, &ts);
	localtime_r(&ts.tv_sec, &tm);

	strftime(buf, sizeof(buf), "%T", &tm);
	fprintf(logger_fp, "%s %06ld <%s:%d> ",
			buf, ts.tv_nsec / 1000, func, line);

	va_start(ap, fmt);
	vfprintf(logger_fp, fmt, ap);
	va_end(ap);

	fprintf(logger_fp, "\n");

	errno = error;
}

void
logger_hexdump(const char *func, int line, const char *ident,
		const void *data, size_t size)
{
	char buf[32];
	char *p;
	struct timespec ts;
	struct tm tm;
	int error;
	size_t i;
	size_t end;
	int c;

	error = errno;

	if (logger_fp == NULL) {
		logger_fp = fopen(logger_file, "a");
		if (logger_fp == NULL) {
			errno = error;
			return;
		}
		setvbuf(logger_fp, NULL, _IONBF, 0);
	}

	clock_gettime(CLOCK_REALTIME, &ts);
	localtime_r(&ts.tv_sec, &tm);

	strftime(buf, sizeof(buf), "%T", &tm);
	fprintf(logger_fp, "%s %06ld <%s:%d> %s ---\n",
			buf, ts.tv_nsec / 1000, func, line, ident);

	end = (size + 0xf) &~ 0xf;
	p = buf;
	for (i = 0; i < end; i++) {
		if ((i & 0xf) == 0)
			fprintf(logger_fp, "%04zu:", i);
		if (i < size) {
			c = *((uint8_t *)data + i);
			fprintf(logger_fp, " %02x", c);
			*p++ = isprint(c) ? c : '.';
		} else {
			fprintf(logger_fp, "   ");
		}
		if ((i & 0xf) == 0xf) {
			*p = '\0';
			fprintf(logger_fp, " |%s|\n", buf);
			p = buf;
		}
	}

	fprintf(logger_fp, "---\n");

	errno = error;
}
