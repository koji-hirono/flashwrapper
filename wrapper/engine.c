#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>

#include "engine.h"


int
engine_open(Engine *e, const char *path, const char *plugin)
{
	int fds[2];
	char name[32];
	char *argv[] = {
		path,
		"-s",
		name,
		"-p",
		plugin,
		"-v",
		NULL
	};

	if (socketpair(PF_LOCAL, SOCK_STREAM, 0, fds) != 0)
		return -1;

	snprintf(name, sizeof(name), "%d", fds[0]);

	e->pid = fork();
	if (e->pid < 0) {
		close(fds[0]);
		close(fds[1]);
		return -1;
	}
	if (e->pid == 0) {
		close(fds[1]);
		execv(path, argv);
		_exit(0);
	}
	close(fds[0]);
	e->fd = fds[1];

	return 0;
}

void
engine_close(Engine *e)
{
	int status;
	int ret;

	close(e->fd);

	ret = waitpid(e->pid, &status, WNOHANG);
	if (ret == 0) {
		kill(e->pid, SIGTERM);
		sleep(1);
		ret = waitpid(e->pid, &status, WNOHANG);
		if (ret == 0) {
			kill(e->pid, SIGKILL);
			sleep(1);
			waitpid(e->pid, &status, WNOHANG);
		}
	}
}

int
engine_send(Engine *e, const void *data, size_t size)
{
	const char *p = data;
	size_t len = size;
	int r;

	do {
		r = write(e->fd, p, len);
		if (r == -1)
			return -1;
		len -= r;
		p += r;
	} while (len != 0);

	return 0;
}

int
engine_recv(Engine *e, void *data, size_t size)
{
	char *p = data;
	size_t len = size;
	int r;

	do {
		r = read(e->fd, p, len);
		if (r == -1)
			return -1;
		if (r == 0)
			return -1;
		len -= r;
		p += r;
	} while (len != 0);

	return 0;
}
