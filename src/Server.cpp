#include <cstring>
#include <signal.h>

#include "Server.h"

/**
 * Создание сервера TCP (прослушивающего дискриптора).
 * Выполняются стандартные 3 этапа:
 * - socket
 * - bind
 * - listen
 */
Server::Server(Endpoint&& endpoint, SocketType socketType)
	: m_serverSocket(Socket{std::move(endpoint), socketType})
{
}

int Server::Accept(struct sockaddr_in& client_addr) {
	return m_serverSocket.Accept(client_addr);
}

/**
 * Определение адреса и порта клиента
 */
std::pair<std::string, uint16_t> Server::GetClientId(const struct sockaddr_in& client) {
	std::pair<std::string, int> res;
	char client_addr[INET_ADDRSTRLEN];	//	адрес клиента. То же самое, что и возвращаемое значение. Не исп-ся!
	//	преобразуем 32-битовый адрес в строку
	res.first = inet_ntop(client.sin_family, &client.sin_addr, client_addr, sizeof(client_addr));
	res.second = ntohs(client.sin_port);	//	преобразование сетевого порядка байтов в порядок байт узла

	return res;
}

/**
 * Завершение соединения с клиентом.
 */
void Server::Close(int connfd) {
	shutdown(connfd, SHUT_RDWR);	//	разрываем соединение с клиентом на чтение и запись
	close(connfd);
}

/**
 * Передача клиенту ответа от Сервера
 */
void Write(int connfd, const std::string& writeString) {
	send(connfd, writeString.c_str(), writeString.size(), MSG_NOSIGNAL);
}

/**
 * Чтение n - байт по одному.
 * В сетевых программах могут быть случаи, когда за один раз не получится все прочитать.
 */
ssize_t Server::readn(int fd, void *vptr, size_t n) {
	size_t	nleft = 0;	//	оставшееся кол-во читаемых байт
	ssize_t	nread = 0;	//	кол-во считанных байт на данный мом-т
	char	*ptr;		//	ук-ль на читаемый буфер

	bzero(vptr, n);
	ptr = (char*)vptr;
	nleft = n;
	while (nleft > 0) {
		nread = recv(fd, ptr, nleft, MSG_NOSIGNAL);
		if (nread < 0) {
			if (errno == EINTR) {
				/* and call read() again */
				continue;
			} else {
				return(-1);
			}
		} else if (nread == 0) {
			break;	/* EOF */
		}
		else {
			if (ptr[nread - 1] == '\n') {
				nleft -= nread;	//	количество читаемых байт уменьшается на кол-во прочитанных байт
				break;	/* EOF */
			}
		}

		nleft -= nread;	//	количество читаемых байт уменьшается на кол-во прочитанных байт
		ptr   += nread;	//	указатель сдвигается на кол-во прочитанных байт
	}
	return(n - nleft);		/* return >= 0 */
}

ssize_t Server::Readn(int listenFd, void *ptr, size_t nbytes) {
	ssize_t	n;
	if ((n = readn(listenFd, ptr, nbytes)) < 0) {
		std::cerr << "readn error: " << strerror(errno) << std::endl;
	}

	return(n);
}

/**
 * Запись n байт.
 * Вариант для неблокирующего write, но годится и для неблокирующего
 */
ssize_t Server::writen(int fd, const void *vptr, size_t n) {
	size_t		nleft;		//	оставшееся кол-во записываемых байт
	ssize_t		nwritten;	//	кол-во записанных байт на данный мом-т
	const char	*ptr;		//	ук-ль на записываемый буфер

	ptr = (char*)vptr;
	nleft = n;
	while (nleft > 0) {
		if ((nwritten = write(fd, ptr, nleft)) < 0) {
			if (errno == EINTR) {
				continue;		/* and call write() again */
			}
			return -1;			/* error */
		}

		nleft -= nwritten;	//	количество записываемых байт уменьшается на кол-во записанных байт
		ptr   += nwritten;	//	указатель сдвигается на кол-во записанных байт
	}
	return(n);
}

void Server::Writen(int fd, const void *ptr, size_t nbytes) {
	if (writen(fd, ptr, nbytes) != static_cast<int>(nbytes)) {
		std::cerr << "writen error: " << strerror(errno) << std::endl;
	}
}

/**
 * Ф-я отправки эха-сообщения клиенту
 */
void Server::StrEcho(int clientfd) {
	char buf[128];
	int n = Readn(clientfd, buf, sizeof(buf));
	//cout << "Read: " << buf << endl;
	Writen(clientfd, buf, n-1);
	//cout << "Write: " << buf << endl;
}

/**
 * Обертка для ф-ции sigaction.
 * Первый арг-т - имя сигнала.
 * Второй арг0т ф-я обработчик этого сигнала.
 */
void Server::SignalInit(int signo, void (*signal_handler)(int)) {
    struct sigaction action;
    action.sa_handler = signal_handler;
    //	установка маски обработчика сигнала. Во время работы обработчика дополнительные сигналы не блокируются
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction(signo, &action, NULL) == -1) {
        std::cerr << "sigaction: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
}
