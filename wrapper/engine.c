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
	int len;
	int fds[2];
	int rfds[2];
	char name[32];
	char rname[32];
	char *const argv[] = {
		path,
		"-s",
		name,
		"-r",
		rname,
		"-p",
		plugin,
		"-v",
		NULL
	};

	if (socketpair(PF_LOCAL, SOCK_STREAM, 0, fds) != 0)
		return -1;

	if (socketpair(PF_LOCAL, SOCK_STREAM, 0, rfds) != 0) {
		close(fds[0]);
		close(fds[1]);
		return -1;
	}

	snprintf(name, sizeof(name), "%d", fds[0]);
	snprintf(rname, sizeof(rname), "%d", rfds[0]);

	len = 65536 * 2;
	if (setsockopt(fds[1], SOL_SOCKET, SO_SNDBUF, &len, sizeof(len)) != 0) {
		close(fds[0]);
		close(fds[1]);
		return -1;
	}

	e->pid = fork();
	if (e->pid < 0) {
		close(fds[0]);
		close(fds[1]);
		close(rfds[0]);
		close(rfds[1]);
		return -1;
	}
	if (e->pid == 0) {
		close(fds[1]);
		close(rfds[1]);
		execv(path, argv);
		_exit(0);
	}
	close(fds[0]);
	close(rfds[0]);
	e->fd = fds[1];
	e->rfd = rfds[1];

	return 0;
}

void
engine_close(Engine *e)
{
	int status;
	int ret;

	close(e->fd);
	close(e->rfd);

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
