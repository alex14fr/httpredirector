#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <string.h>
#include <errno.h>

#define REDIRECT_TO "https://uq.math.cnrs.fr"
#define MAXLEN 8000

#define mywrstr(fd, str) write((fd), (str), strlen(str));

void badreq(int fd) {
	mywrstr(fd, "HTTP/1.1 400\r\n\r\n");
	exit(0);
}

int myreadline(int fd, char *line) {
	int nread=0;
	while(nread<MAXLEN) {
		int nr=read(fd, line+nread, MAXLEN-nread);
		if(nr<=0) {
			exit(0);
		}
		for(int i=nread; i<nread+nr; i++) {
			if(line[i]=='\n') {
				return(i);
			}
		}
		nread+=nr;
	}
	badreq(fd);
	return(0);
}

void alarmhdl(int s) {
	exit(0);
}

void handle_conn(int sock, struct sockaddr_in6 *saddr) {
	char line[MAXLEN];
	alarm(60);
	signal(SIGALRM, alarmhdl);
	while(1) {
		int n, i=0;
		n=myreadline(sock, line);
		while(i<n && line[i]!=' ') i++;
		while(i<n && line[i]==' ') i++;
		if(i==n) badreq(sock);
		const char * const url=line+i;
		int urllen;
		while(i<n && line[i]!='\r' && line[i]!='\n' && line[i]!=' ') i++;
		urllen=i-(url-line);
		mywrstr(sock, "HTTP/1.1 301\r\nLocation: " REDIRECT_TO);
		write(sock, url, urllen);
		mywrstr(sock, "\r\nConnection: close\r\n\r\n");
		exit(0);
	}
}

void server(int argc, char **argv) {
	int ssock=socket(AF_INET6, SOCK_STREAM, 0);
	struct sockaddr_in6 addr, addr_c;
	memset(&addr, 0, sizeof(struct sockaddr_in6));
	addr.sin6_family=AF_INET6;
	*((unsigned char*)&addr.sin6_port)=0x1f; 
	*(1+(unsigned char*)&addr.sin6_port)=0x90;
/*	int z=1;
	setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &z, sizeof(int)); */
	if(ssock < 0 ||
		bind(ssock, (struct sockaddr*)(&addr), sizeof(struct sockaddr_in6)) < 0 ||
		listen(ssock, 1) < 0) { 
			goto end;
	}
	int csock;
	socklen_t x=sizeof(struct sockaddr_in6);
	while((csock=accept(ssock, (struct sockaddr*)(&addr_c), &x))) {
		if(fork()==0) {
			handle_conn(csock, &addr_c);
		} else {
			close(csock);
		}
	}
	end: 
	close(ssock);
	return;
}

int main(int argc, char **argv) {
	signal(SIGCHLD, SIG_IGN);
	server(argc, argv);
	return(0);
}
