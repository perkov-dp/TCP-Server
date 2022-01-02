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
 * Связывание заранее известного порта сервера с сокетом
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
 */
int Server::Accept() {
	socklen_t size = sizeof(servaddr);
	int res = accept(listenFd, (struct sockaddr*)&servaddr, &size);
	if (res == -1) {
		perror("SERVER Accept() failed");
	}
	return res;
}

/**
 * Передача клиенту ответа от Сервера
 */
void Server::Write(int connfd, const string& write_string) {
	write(connfd, write_string.c_str(), write_string.size());
}

/**
 * Завершение соединения с клиентом.
 */
void Server::Close(int connfd) {
	close(connfd);
}