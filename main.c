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
#define MAXLEN 1024


struct myrbuf {
	char b[MAXLEN];
	int i;
	int l;
	int fd;
};

#define myperr(s) { write(STDERR_FILENO, (s), strlen(s)); }
#define mywrstr(x, str) write((x)->fd, (str), strlen(str));

int mygetc(struct myrbuf *rb) {
	if(rb->i >= rb->l) {
		int nr;
		nr=read(rb->fd, rb->b, MAXLEN);
		if(nr <= 0) 
			return(-1);
		rb->i=0;
		rb->l=nr;
	}
	return(rb->b[rb->i++]);
}

void badreq(struct myrbuf *rb) {
	mywrstr(rb, "HTTP/1.1 400 Bad request\r\n\r\n");
	exit(0);
}

int myreadline(struct myrbuf *rb, char *line) {
	int n=0;
	while(1) {
		if(n>=MAXLEN) badreq(rb);
		line[n++]=mygetc(rb);
		if(n>=2 && line[n-2]=='\r' && line[n-1]=='\n') 
			break;
		else if((n==1 && line[0]=='\n') || (n>=2 && line[n-2]!='\r' && line[n-1]=='\n')) 
			badreq(rb);
	}
	line[n]=0;
	return(n);
}

void alarmhdl(int s) {
	exit(0);
}

void handle_conn(int sock, struct sockaddr_in6 *saddr) {
	struct myrbuf rb;
	rb.i=rb.l=0;
	rb.fd=sock;
	char line[MAXLEN];
	alarm(60);
	signal(SIGALRM, alarmhdl);
	while(1) {
		int n, i=0;
		n=myreadline(&rb, line);
		while(i<n && line[i]!=' ') i++;
		if(i==n) badreq(&rb);
		i++;
		char *url;
		int urllen;
		url=line+i;
		while(i<n && line[i]!='\r' && line[i]!=' ') i++;
		urllen=i-(url-line);
		mywrstr(&rb, "HTTP/1.1 301 Moved permanently\r\nLocation: " REDIRECT_TO);
		write(rb.fd, url, urllen);
		mywrstr(&rb, "\r\nConnection: close\r\n\r\n");
		exit(0);
	}
}

inline void server(int argc, char **argv) {
	int ssock=socket(AF_INET6, SOCK_STREAM, 0);
	struct sockaddr_in6 addr, addr_c;
	memset(&addr, 0, sizeof(struct sockaddr_in6));
	addr.sin6_family=AF_INET6;
	*((char*)&addr.sin6_port)=0x1f; 
	*(1+(char*)&addr.sin6_port)=0x90;
/*	int z=1;
	setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &z, sizeof(int)); */
	if(bind(ssock, (struct sockaddr*)(&addr), sizeof(struct sockaddr_in6))<0) { myperr("bind"); return; }
	if(listen(ssock, 1)<0) { myperr("listen"); return; }
	int csock;
	socklen_t x=sizeof(struct sockaddr_in6);
	while((csock=accept(ssock, (struct sockaddr*)(&addr_c), &x))) {
		if(fork()==0) {
			handle_conn(csock, &addr_c);
		} else {
			close(csock);
		}
	}
}

int main(int argc, char **argv) {
	signal(SIGCHLD, SIG_IGN);
	server(argc, argv);
	return(0);
}
