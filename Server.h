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

/**
 * Макс кол-во клиентских соед-ий, к-рые ядро
 * ставит в очередь на прослушиваемом сокете.
 */
const size_t LISTENQ = 1024;

class Server {
public:
	Server(uint16_t port_number);
	int Accept();
	void Write(int connfd, const string& write_string);
	void Close(int connfd);
private:
	int Socket(int family, int socket_type);
	sockaddr_in InitSockaddrStruct(int family, uint32_t hostlong, uint16_t port_number);
	void Bind(int sockfd, const struct sockaddr_in& servaddr);
	void Listen(int sockfd, size_t listen_queue_size);

	int listenFd;
	struct sockaddr_in servaddr;
};
