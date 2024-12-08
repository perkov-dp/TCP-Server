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
Server::Server(SocketType socketType, uint16_t portNumber) {
	/**
	 * Создание сокета 'SocketType'.
	 * Ф-я Socket возвращает дискриптор, к-рый идентифицирует сокет
	 * в последующих вызовах (connect, read)
	 */
	const int PROTOCOL_FAMILY = AF_INET;	//	семейство протоколов IPv4
	m_listenFd = Socket(PROTOCOL_FAMILY, socketType);

	/**
	 * Связывание заранее известного порта сервера с сокетом
	 */
	constexpr uint32_t IP_ADDRESS = INADDR_ANY;	//	прием запроса с любого адреса
	m_servaddr = InitSockaddrStruct(PROTOCOL_FAMILY, IP_ADDRESS, portNumber);
	Bind(m_listenFd, m_servaddr);

	/**
	 * Преобразование сокета в прослушиваемый сокет, т.е. такой,
	 * на к-ром ядро принимает входящие сообщ-я от клиентов.
	 * Второй пар-р задает макс кол-во клиентских соед-ий, к-рые ядро
	 * ставит в очередь на прослушиваемом сокете.
	 */
	Listen(m_listenFd, LISTENQ);
}

Server::~Server() {
	Close(m_listenFd);
}


/**
 * Создание сокета. Сокетом часто называют два значения,
 * идентифицирующие конечную точку: IP-адрес и номер порта.
 *
 * Принимает на вход:
 * - семейство протоколов (AF_INET - интернета IPv4)
 * - type (SOCK_STREAM - потоковый сокет)
 * - protocol - обычно 0
 * Все вместе - это название TCP-сокета
 * Возвращает дескриптор сокета
 */
int Server::Socket(int family, SocketType type) {
	int sockfd = 0;
	if ((sockfd = socket(family, static_cast<int>(type), 0)) < 0) {
		std::cerr << "Socket() error" << std::endl;
		exit(EXIT_FAILURE);
	}

	return sockfd;
}

/**
 * Заполнение структуры адреса Интернета
 * Второй пар-р (INADDR_ANY) позволяет Серверу принимать соединения клиента
 * на любом интерфейсе
 */
sockaddr_in Server::InitSockaddrStruct(int family, uint32_t hostlong, uint16_t portNumber) {
	/**
	 * Заполнение стр-ры адреса сокета Интернета
	 * IP-адресом и № порта сервера
	 * inet_pton - преобразовывает строковый IP-адрес в двоичный формат
	 */
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = family;	//	IPv4
	servaddr.sin_addr.s_addr = htonl(hostlong);
	servaddr.sin_port = htons(portNumber);	//	приводит № порта к нужному формату

	return servaddr;
}

/**
 * Связывание заранее сокет c IP-адресом.
 * Для вызова bind н/обладать правами суперпользователя!!!
 */
void Server::Bind(int sockfd, const struct sockaddr_in& servaddr) {
	if ((bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) == -1) {
		std::cerr << "SERVER Bind() failed" << std::endl;
		exit(EXIT_FAILURE);
	}
}

/**
 * Преобразование сокета в прослушиваемый сокет, т.е. такой,
 * на к-ром ядро принимает входящие сообщ-я от клиентов.
 * Второй пар-р задает макс кол-во клиентских соед-ий, к-рые ядро
 * ставит в очередь на прослушиваемом сокете.
 */
void Server::Listen(int sockfd, size_t listenQueueSize) {
	if ((listen(sockfd, listenQueueSize) == -1)) {
		std::cerr << "SERVER Listen() failed" << std::endl;
		exit(EXIT_FAILURE);
	}
}


/**
 * Прием клиентского соединения.
 * Процесс блокируется ф-ей Accept, ожидая принятия подключения клиента.
 * После установления соединения Accept возвращает значение нового дескриптора,
 * который называется присоединенным дескриптором.
 * Этот дескриптор исп-ся для связи с новым клиентом и возвращается для каждого клиента,
 * соединяющегося с нашим сервером.
 *
 * Аргументы servaddr и size исп-ся для идентификации клиента (адрес протокола клиента)
 */
int Server::Accept(struct sockaddr_in& client_addr) {
	socklen_t size = sizeof(client_addr);
	int clientFd = accept(m_listenFd, (struct sockaddr*)&client_addr, &size);
	if (clientFd == -1 && errno != EINTR) {
		std::cerr << "SERVER Accept() failed" << std::endl;
		exit(EXIT_FAILURE);
	}
	//	Во время вып-я Accept пришел сигнал и прервал ее выполнение -> перезапускаем Accept
	else if (errno == EINTR) {
		return -1;
	}

	return clientFd;
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
 * Передача клиенту ответа от Сервера
 */
void Write(int connfd, const std::string& writeString) {
	write(connfd, writeString.c_str(), writeString.size());
}

/**
 * Завершение соединения с клиентом.
 */
void Server::Close(int connfd) {
	close(connfd);
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
		nread = read(fd, ptr, nleft);
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
		std::cerr << "readn error" << std::endl;
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
		std::cerr << "writen error" << std::endl;
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
        std::cerr << "sigaction" << std::endl;
        exit(EXIT_FAILURE);
    }
}
