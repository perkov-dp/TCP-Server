#pragma once
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>

#include <sys/types.h>
#include <unistd.h>	//	read
#include <netinet/in.h>	//	sockaddr_in
#include <sys/socket.h>	//	socket
#include <arpa/inet.h>	//	inet_ntop
#include <errno.h>
#include <stdlib.h>		//	exit

#include "Socket.h"

class Server {
public:
	Server(Endpoint&& endpoint, SocketType socketType);
	
	int Accept(struct sockaddr_in& client_addr);
	std::pair<std::string, uint16_t> GetClientId(const struct sockaddr_in& client);
	void Close(int connfd);

	ssize_t Readn(int listenFd, void *vptr, size_t n);
	void Writen(int connfd, const void *vptr, size_t n);
	void StrEcho(int clientfd);

	int GetListenFd() {
		return m_listenFd;
	}

	void SignalInit(int signo, void (*signal_handler)(int));
private:
	ssize_t readn(int fd, void *vptr, size_t n);
	ssize_t writen(int fd, const void *vptr, size_t n);

	int m_listenFd;
	struct sockaddr_in m_servaddr;

	Endpoint m_address;

	Socket m_serverSocket;
};
