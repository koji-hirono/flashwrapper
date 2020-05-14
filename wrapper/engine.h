#ifndef ENGINE_H__
#define ENGINE_H__

#include <unistd.h>

typedef struct Engine Engine;

struct Engine {
	int fd;
	pid_t pid;
};

extern int engine_open(Engine *, const char *, const char *);
extern void engine_close(Engine *);
extern int engine_send(Engine *, const void *, size_t);
extern int engine_recv(Engine *, void *, size_t);

#endif
