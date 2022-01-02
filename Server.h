#pragma once
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
using namespace std;

#include <sys/types.h>
#include <unistd.h>	//	read
#include <netinet/in.h>	//	sockaddr_in
#include <sys/socket.h>	//	socket
#include <arpa/inet.h>	//	inet_ntop
#include <errno.h>

/**
 * Макс кол-во клиентских соед-ий, к-рые ядро
 * ставит в очередь на прослушиваемом сокете.
 */
const size_t LISTENQ = 1024;

class Server {
public:
	Server(uint16_t port_number);
	int Accept(struct sockaddr_in& client_addr);
	pair<string, uint16_t> GetClientId(const struct sockaddr_in &client);
	void Close(int connfd);

	ssize_t Readn(void *vptr, size_t n);
	void Writen(int connfd, const void *vptr, size_t n);
private:
	int Socket(int family, int socket_type);
	sockaddr_in InitSockaddrStruct(int family, uint32_t hostlong, uint16_t port_number);
	void Bind(int sockfd, const struct sockaddr_in& servaddr);
	void Listen(int sockfd, size_t listen_queue_size);

	ssize_t readn(int fd, void *vptr, size_t n);
	ssize_t writen(int fd, const void *vptr, size_t n);

	int listenFd;
	struct sockaddr_in servaddr;
};
