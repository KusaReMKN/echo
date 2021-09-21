#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netdb.h>

#ifdef __GNUC__
#define NORETURN __attribute__((noreturn))
#else
#define NORETURN
#endif

#define VERSION "0.00"

#define BACKLOG 5
#define SERVICE "echo"

void sigchld_handler(int sig);

int listen_socket(const char *service);
void echomain(int wsock) NORETURN;

int main(int argc, char *argv[])
{
	int sock, wsock;
	char *service = SERVICE;
	struct sigaction sa;

	char hoge[1024] = "echo ";

	if (argc > 2) {
		fprintf(stderr, "echo daemon (Version: %s)\n", VERSION);
		fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		return 1;
	}
	if (argc == 2)
		service = argv[1];

	/*
	 *  TODO: sigaction
	 *  for SIGINT
	 */
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;	/* for accept(2) */
	sa.sa_handler = sigchld_handler;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction(SIGCHLD)");
		return 1;
	}

	sock = listen_socket(service);
	if (sock == -1) {
		fprintf(stderr, "listen_socket: Failed\n");
		return 1;
	}

	if (daemon(0, 0) == -1) {
		perror("daemon");
		return 1;
	}

	while (~(wsock = accept(sock, NULL, NULL))) {
		if (fork() == 0) {
			close(sock);
			echomain(wsock);
		} else {
			close(wsock);
		}
	}

	/* NOT REACHED */
	system(strcat(strcat(hoge, strerror(errno)), " >~/echod.err"));
	close(sock);
	return -1;
}

void sigchld_handler(int sig)
{
	while (waitpid(-1, NULL, WNOHANG) > 0);
}


int listen_socket(const char *service)
{
	int err;
	int sock, soval;
	struct addrinfo hints = { 0 };
	struct addrinfo *result, *rp;

	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags    = AI_PASSIVE;
	err = getaddrinfo(NULL, service, &hints, &result);
	if (err != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
		return -1;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sock == -1)
			continue;
		soval = 1;
		err = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &soval,
								sizeof(soval));
		if (err == -1) {
			close(sock);
			continue;
		}
		if (bind(sock, rp->ai_addr, rp->ai_addrlen) == -1) {
			close(sock);
			continue;
		}
		if (listen(sock, BACKLOG) == 0)
			break;	/* success */
		close(sock);
	}
	freeaddrinfo(result);
	if (rp == NULL) {
		fprintf(stderr, "%s: Could not listen\n", __FUNCTION__);
		return -1;
	}
	return sock;
}

#define BUFFSIZE 1024

void echomain(int wsock)
{
	char buf[BUFFSIZE];
	int len;

	while ((len = read(wsock, buf, sizeof(buf))) > 0)
		write(wsock, buf, len);

	shutdown(wsock, SHUT_RDWR);
	close(wsock);
	_exit(0);
	/* NOT REACHED */
}
