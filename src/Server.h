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

/**
 * Макс кол-во клиентских соед-ий, к-рые ядро
 * ставит в очередь на прослушиваемом сокете.
 */
constexpr size_t LISTENQ = 1024;

enum class SocketType {
	SOCK_STREAM,
	SOCK_DGRAM
};

class Server {
public:
	Server(SocketType socketType, uint16_t portNumber);
	virtual ~Server();
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
	int Socket(int family, SocketType socketType);
	sockaddr_in InitSockaddrStruct(int family, uint32_t hostlong, uint16_t portNumber);
	void Bind(int sockfd, const struct sockaddr_in& servaddr);
	void Listen(int sockfd, size_t listenQueueSize);

	ssize_t readn(int fd, void *vptr, size_t n);
	ssize_t writen(int fd, const void *vptr, size_t n);

	int m_listenFd;
	struct sockaddr_in m_servaddr;
};
