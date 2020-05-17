#ifndef ENGINE_H__
#define ENGINE_H__

#include <unistd.h>

typedef struct Engine Engine;

struct Engine {
	int fd;
	int rfd;
	pid_t pid;
};

extern int engine_open(Engine *, const char *, const char *);
extern void engine_close(Engine *);

#endif
