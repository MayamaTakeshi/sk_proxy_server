#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdarg.h>

#include "brs_stream_utils.h"
#include "switchkitjson.h"

char json_buf[8912];

void waitchildren(int signum) {
	pid_t pid;
	while((pid == waitpid(-1, NULL, WNOHANG)) > 0) {
		printf("Caught the exit fo child process %d.\n", pid);
	}
}

pid_t safefork(void);
static int connectioncount = 0;

const int port = 1313;

void print_func_error(const char *func_name, int ecode, const char *fmt, ...) {
	va_list fmtargs;
	fprintf(stderr, "Call to %s failed", func_name);
	va_start(fmtargs, fmt);
	fprintf(stderr, fmt, fmtargs);
	va_end(fmtargs);
	fprintf(stderr, ". err=%d strerror=%s.\n", ecode, strerror(ecode));

}

void child_process(int sock);
	
int main(void) {
	int listensock; 
	int workersock;
	struct protoent *protocol;
	struct sockaddr_in socketaddr;
	int addrlen;
	int trueval =1;
	struct sigaction act;

	int on = 1;

	int retval;

	/* initialize the signal handler */
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_RESTART;
	act.sa_handler = (void*)waitchildren;
	sigaction(SIGCHLD, &act, NULL);

	/* generate the socket structure and resolve names */
	bzero((char*)&socketaddr, sizeof(socketaddr));
	socketaddr.sin_family = AF_INET;
	socketaddr.sin_addr.s_addr = INADDR_ANY;
	socketaddr.sin_port = htons(1313);

	/* resolve protocol name */
	protocol = getprotobyname("tcp");
	if(!protocol) {
		print_func_error("getprotobyname", errno, "");
		exit(1);
	}	

	/* create master socket */
	listensock = socket(PF_INET, SOCK_STREAM, protocol->p_proto);
	if(listensock < 0) {
		print_func_error("socket", errno, "");
		exit(1);
	}

	if (setsockopt(listensock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
		print_func_error("setsockopt", errno, "");
		exit(1);
	}

	/* bind to port */
	if(bind(listensock, &socketaddr, sizeof(socketaddr)) < 0) {
		print_func_error("bind", errno, "");
		exit(1);
	}
	
	/* listen for connections */
	if(listen(listensock, 0) < 0) {
		print_func_error("listen", errno, "");
		exit(1);
	}	

	while(1) {
		workersock = accept(listensock, &socketaddr, &addrlen);
		if(workersock < 0) {
			print_func_error("accept", errno, "");
			exit(1);
		}

		connectioncount++;

		retval = fork();
		if(retval == -1) {
			print_func_error("fork", errno, "");
			exit(1);
		}
		
		if(retval) {
			/* parent process */
			if(close(workersock) != 0) {
				print_func_error("close", errno, "Parent failed to close workersock");
			}
		} else {
			/* child process */
			if(close(listensock) != 0) {
				print_func_error("close", errno, "Child failed to close listensock");
			}
			child_process(workersock);
			exit(0);
		}

	}
	return 0;
}

int initialize(int fd, void *data, char *line, int len) {
	int *pInitialized = (int*)data;
	char *p;
	char *appName;
	char *appVersion;
	char *appDescription;
	int instanceId;
	char *host;
	int port;

	char *reply;

	if(len == 0) {
		// apps like telnet will insert \r\n as line termination and so empty lines might show up. So just ignore.
		return 1;
	}

	p = strtok(line, " ");
	if(!p) {
		return 0;
	}
	if(strcmp("skj_initialize", p) != 0) {
		reply = "{\"event\": \"handshake_failed\", \"Reason\": \"no skj_initialize specified\"}\n";
		write(fd, reply, strlen(reply));
		return 0;
	}

	p = strtok(NULL, " ");
	if(!p) {
		reply = "{\"event\": \"handshake_failed\", \"Reason\": \"no appName specified\"}\n";
		write(fd, reply, strlen(reply));
		return 0;
	}
	appName = p;

	p = strtok(NULL, " ");
	if(!p) {
		reply = "{\"event\": \"handshake_failed\", \"Reason\": \"no appVersion specified\"}\n";
		write(fd, reply, strlen(reply));
		return 0;
	}
	appVersion = p;

	p = strtok(NULL, " ");
	if(!p) {
		reply = "{\"event\": \"handshake_failed\", \"Reason\": \"no appDescription specified\"}\n";
		write(fd, reply, strlen(reply));
		return 0;
	}
	appDescription = p;

	p = strtok(NULL, " ");
	if(!p) {
		reply = "{\"event\": \"handshake_failed\", \"Reason\": \"no instanceId specified\"}\n";
		write(fd, reply, strlen(reply));
		return 0;
	}
	instanceId = atoi(p);

	p = strtok(NULL, " ");
	if(!p) {
		reply = "{\"event\": \"handshake_failed\", \"Reason\": \"no host specified\"}\n";
		write(fd, reply, strlen(reply));
		return 0;
	}
	host = p;

	p = strtok(NULL, " ");
	if(!p) {
		reply = "{\"event\": \"handshake_failed\", \"Reason\": \"no port specified\"}\n";
		write(fd, reply, strlen(reply));
		return 0;
	}
	port = atoi(p);

	reply = "{\"event\": \"handshake_ok\"}\n";
	write(fd, reply, strlen(reply));

	int sz = sizeof(json_buf);
	int ret = skj_initialize(json_buf, &sz, appName, appVersion, appDescription, instanceId, host, port);
	if(!ret) {
		printf("skj_initialize failed\n");
		json_buf[sz] = '\n';
		write(fd, json_buf, sz+1);
		return 0;
	}

	*pInitialized = 1;
	printf("skj_initialize ok\n");
	json_buf[sz] = '\n';
	write(fd, json_buf, sz+1);
	return 1;
}

int process_request(int fd, void *data, char *line, int len) {
	if(len == 0) {
		// empty line
		return 1;
	}
	int sz;

	sz = sizeof(json_buf);
	skj_exec(json_buf, &sz, line);
	json_buf[sz] = '\n';

	if(write(fd, json_buf, sz+1) > 0) {
		// OK
		return 1;
	} else {
		//Failed
		return 0;
	}
}

void child_process(int sock) {
	int initialized = 0;
	int sk_sock = 0;
	fd_set fdset;
	static char buf[8192];
	int buf_sz = sizeof(buf);

	int ret;

	stream_framer_t sf;
	sf.buf = buf;
	sf.buf_sz = buf_sz;
	sf.len = 0;
	sf.fd = sock;

	while(1) {
		FD_ZERO(&fdset);

		FD_SET(sock, &fdset);

		if(initialized) {
			sk_sock = skj_getLLCSocketDescriptor();
			FD_SET(sk_sock, &fdset);	
		}

		int rv = select(FD_SETSIZE, &fdset, NULL, NULL, NULL);
		if(rv < 0) {
			; //Failed. Better luck next time
		} else if (!rv) {
			; //Timeout
		} else {
			if(FD_ISSET(sock, &fdset)) {
				if(!initialized) {
					//printf("calling initialize\n");
					ret = recv_and_dispatch_line(&sf, recv, initialize, (void*)&initialized);		
				} else {
					//printf("calling recv_and_dispatch_line\n");
					ret = recv_and_dispatch_line(&sf, recv, process_request, (void*)0);		
				}

				//printf("recv_and_dispatch ret=%i\n", ret);
				if(ret != STREAM_FRAMER_STATUS_OK) {
					skj_closeConnection();
					exit(0);	
				}
				
			}

			if(initialized && sk_sock && FD_ISSET(sk_sock, &fdset)) {
				int sz;
				sz = sizeof(json_buf);
				skj_poll(json_buf, &sz);
				if(json_buf[0]) {
					//printf("poll got something: %s\n", json_buf);
					//printf("size=%i\n", sz);
					json_buf[sz] = '\n';
					int ret = write(sock, json_buf, sz+1);
					//int ret = send(sock, json_buf, sz+1, 0);
					if(ret < 0) {
						//error
						printf("write failed after skj_poll ret=%i\n", ret);
						//printf("send failed after skj_poll ret=%i\n", ret);
						close(sock);
						exit(0);
					}
				}
			}
		} 
	}	
}
