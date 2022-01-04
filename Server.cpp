#include "Server.h"

/**
 * Создание сервера TCP (прослушивающего дискриптора).
 * Выполняются стандартные 3 этапа:
 * - socket
 * - bind
 * - listen
 */
Server::Server(uint16_t port_number) {
	/**
	 * Создание сокета TCP.
	 * Ф-я Socket возвращает дискриптор, к-рый идентифицирует сокет
	 * в последующих вызовах (connect, read)
	 */
	const int SOCKET_TYPE = SOCK_STREAM;	//	потоковый сокет
	const int PROTOCOL_FAMILY = AF_INET;	//	семейство протоколов IPv4
	listenFd = Socket(PROTOCOL_FAMILY, SOCKET_TYPE);

	/**
	 * Связывание заранее известного порта сервера с сокетом
	 */
	const uint32_t IP_ADDRESS = INADDR_ANY;	//	прием запроса с любого адреса
	servaddr = InitSockaddrStruct(PROTOCOL_FAMILY, IP_ADDRESS, port_number);
	Bind(listenFd, servaddr);

	/**
	 * Преобразование сокета в прослушиваемый сокет, т.е. такой,
	 * на к-ром ядро принимает входящие сообщ-я от клиентов.
	 * Второй пар-р задает макс кол-во клиентских соед-ий, к-рые ядро
	 * ставит в очередь на прослушиваемом сокете.
	 */
	Listen(listenFd, LISTENQ);
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
int Server::Socket(int family, int type) {
	int sockfd = 0;
	if ((sockfd = socket(family, type, 0)) < 0) {
		perror("Socket() error");
		exit(EXIT_FAILURE);
	}

	return sockfd;
}

/**
 * Заполнение структуры адреса Интернета
 * Второй пар-р (INADDR_ANY) позволяет Серверу принимать соединения клиента
 * на любом интерфейсе
 */
sockaddr_in Server::InitSockaddrStruct(int family, uint32_t hostlong, uint16_t port_number) {
	/**
	 * Заполнение стр-ры адреса сокета Интернета
	 * IP-адресом и № порта сервера
	 * inet_pton - преобразовывает строковый IP-адрес в двоичный формат
	 */
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = family;	//	IPv4
	servaddr.sin_addr.s_addr = htonl(hostlong);
	servaddr.sin_port = htons(port_number);	//	приводит № порта к нужному формату

	return servaddr;
}

/**
 * Связывание заранее сокет c IP-адресом.
 * Для вызова bind н/обладать правами суперпользователя!!!
 */
void Server::Bind(int sockfd, const struct sockaddr_in& servaddr) {
	if ((bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) == -1) {
		perror("SERVER Bind() failed");
		exit(EXIT_FAILURE);
	}
}

/**
 * Преобразование сокета в прослушиваемый сокет, т.е. такой,
 * на к-ром ядро принимает входящие сообщ-я от клиентов.
 * Второй пар-р задает макс кол-во клиентских соед-ий, к-рые ядро
 * ставит в очередь на прослушиваемом сокете.
 */
void Server::Listen(int sockfd, size_t listen_queue_size) {
	if ((listen(sockfd, listen_queue_size) == -1)) {
		perror("SERVER Listen() failed");
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
	int client_fd = accept(listenFd, (struct sockaddr*)&client_addr, &size);
	if (client_fd == -1 && errno != EINTR) {
		perror("SERVER Accept() failed");
		exit(EXIT_FAILURE);
	}

	return client_fd;
}

/**
 * Определение адреса и порта клиента
 */
pair<string, uint16_t> Server::GetClientId(const struct sockaddr_in &client) {
	pair <string, int> p;
	char client_addr[INET_ADDRSTRLEN];	//	адрес клиента. То же самое, что и возвращаемое значение. Не исп-ся!
	//	преобразуем 32-битовый адрес в строку
	p.first = inet_ntop(client.sin_family, &client.sin_addr, client_addr, sizeof(client_addr));
	p.second = ntohs(client.sin_port);	//	преобразование сетевого порядка байтов в порядок байт узла

	return p;
}

/**
 * Передача клиенту ответа от Сервера
 */
void Write(int connfd, const string& write_string) {
	write(connfd, write_string.c_str(), write_string.size());
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
		if ( (nread = read(fd, ptr, nleft)) < 0) {
			if (errno == EINTR) {
				/* and call read() again */
				continue;
			}
			return(-1);
		} else if (nread == 0) {
			break;	/* EOF */
		}

		nleft -= nread;	//	количество читаемых байт уменьшается на кол-во прочитанных байт
		ptr   += nread;	//	указатель сдвигается на кол-во прочитанных байт
	}
	return(n - nleft);		/* return >= 0 */
}

ssize_t Server::Readn(int listenFd, void *ptr, size_t nbytes) {
	ssize_t	n;
	if ((n = readn(listenFd, ptr, nbytes)) < 0) {
		perror("readn error");
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
		perror("writen error");
	}
}

/**
 * Ф-я отправки эха-сообщения клиенту
 */
int Server::str_echo(int clientfd) {
	ssize_t		n;
	char		buf[256];

	int result = -1;

	//	пока есть данные, отсылаем их клиенту
	if ((n = Readn(clientfd, buf, sizeof(buf))) > 0) {
		Writen(clientfd, buf, sizeof(buf));
	}
	//	???? -> заново пробуем читать
	if (n < 0 && errno == EINTR) {
		str_echo(clientfd);
	}
	//	клиент разорвал соединение
	else if (n == 0) {
		cout << "str_echo: client close session" << endl;
		result = 0;
	}
	else if (n < 0) {
		perror("str_echo: read error");
		result = -1;
	}

	return result;
}
