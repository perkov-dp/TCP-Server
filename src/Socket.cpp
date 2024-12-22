#include "Socket.h"

#include <arpa/inet.h>	//	inet_ntop
#include <stdexcept>
#include <array>
#include <string.h>
#include <iostream>
#include <unistd.h>


Endpoint::Endpoint(const std::string& addr, uint16_t port)
{
    sockaddr_in addr4 = {};
    sockaddr_in6 addr6 = {};
    if (inet_pton(AF_INET, addr.c_str(), &(addr4.sin_addr)) == 1) {
        addr4.sin_port = htons(port);
        addr4.sin_family = AF_INET;
        m_addr = std::move(addr4);
    } else if (inet_pton(AF_INET6, addr.c_str(), &(addr6.sin6_addr)) == 1) {
        addr6.sin6_port = htons(port);
        addr6.sin6_family = AF_INET6;
        m_addr = std::move(addr6);
    } else {
        throw std::runtime_error("Cannot parse address: '" + addr + "'");
    }
}

Endpoint::Endpoint(const sockaddr_in& addr)
    : m_addr(addr)
{}

Endpoint::Endpoint(sockaddr_in&& addr)
    : m_addr(std::move(addr))
{}

Endpoint::Endpoint(const sockaddr_in6& addr)
    : m_addr(addr)
{}

Endpoint::Endpoint(sockaddr_in6&& addr)
    : m_addr(std::move(addr))
{}

Endpoint::Endpoint(const sockaddr& addr, socklen_t len) {
    if (len == sizeof(sockaddr_in)) {
        sockaddr_in addr4; 
        memcpy(&addr4, &addr, sizeof(addr4));
        m_addr = std::move(addr4);
    } else if (len == sizeof(sockaddr_in6)) {
        sockaddr_in6 addr6; 
        memcpy(&addr6, &addr, sizeof(addr6));
        m_addr = std::move(addr6);
    } else {
        throw std::runtime_error("Bad address size: " + std::to_string(len));
    }
}

int Endpoint::Domain() const {
    if (std::get_if<sockaddr_in>(&m_addr)) {
        return PF_INET;
    } else if (std::get_if<sockaddr_in6>(&m_addr)) {
        return PF_INET6;
    }
    
    return 0;
}

const std::variant<sockaddr_in, sockaddr_in6>& Endpoint::Addr() const { 
    return m_addr; 
}

std::pair<const sockaddr*, int> Endpoint::RawAddr() const {
    if (const auto* val = std::get_if<sockaddr_in>(&m_addr)) {
        return {reinterpret_cast<const sockaddr*>(val), sizeof(sockaddr_in)};
    } else if (const auto* val = std::get_if<sockaddr_in6>(&m_addr)) {
        return {reinterpret_cast<const sockaddr*>(val), sizeof(sockaddr_in6)};
    } else {
        throw std::runtime_error("Empty variant");
    }
}

bool Endpoint::operator == (const Endpoint& other) const {
    return memcmp(&m_addr, &other.m_addr, sizeof(m_addr)) == 0;
}

std::string Endpoint::ToString() const {
    std::array<char, 64> buf;

    if (const auto* val = std::get_if<sockaddr_in>(&m_addr)) {
        if (const auto r = inet_ntop(AF_INET, &val->sin_addr, buf.data(), buf.size())) {
            return std::string(r) + ":" + std::to_string(ntohs(val->sin_port));
        }
    } else if (const auto* val = std::get_if<sockaddr_in6>(&m_addr)) {
        if (const auto r = inet_ntop(AF_INET6, &val->sin6_addr, buf.data(), buf.size())) {
            return "[" + std::string(r) + "]:" + std::to_string(ntohs(val->sin6_port));
        }
    }

    return "";
}

/**
* Создание сокета. Сокетом часто называют два значения,
 * идентифицирующие конечную точку: IP-адрес и номер порта.
 *
 * Принимает на вход:
 * - семейство протоколов (AF_INET - интернета IPv4)
 * - type (SOCK_STREAM - потоковый сокет)
 * Все вместе - это название TCP-сокета
 * Возвращает дескриптор сокета
 */
Socket::Socket(Endpoint&& endpoint, SocketType type) : m_address(std::move(endpoint)) {
    if ((m_listenFd = socket(m_address.Domain(), static_cast<int>(type), 0)) < 0) {
        std::cerr << "Socket() error: " << strerror(errno) << std::endl;
        throw std::system_error(errno, std::generic_category(), "socket");
    }

    /**
	 * Связывание заранее известного порта сервера с сокетом
	 */
	Bind();

	/**
	 * Преобразование сокета в прослушиваемый сокет, т.е. такой,
	 * на к-ром ядро принимает входящие сообщ-я от клиентов.
	 * Второй пар-р задает макс кол-во клиентских соед-ий, к-рые ядро
	 * ставит в очередь на прослушиваемом сокете.
	 */
	Listen(SOMAXCONN);
}

Socket::~Socket() {
    close(m_listenFd);
}

/**
 * Связывание заранее сокет c IP-адресом.
 * Для вызова bind н/обладать правами суперпользователя!!!
 */
void Socket::Bind() {
	const auto [addr, len] = m_address.RawAddr();
    int optval = 1;
    socklen_t optlen = sizeof(optval);
    if (setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR, (char*) &optval, optlen) < 0) {
        throw std::system_error(errno, std::generic_category(), "setsockopt");
    }
    if (bind(m_listenFd, addr, len) < 0) {
        throw std::system_error(errno, std::generic_category(), "bind");
    }
}

/**
 * Преобразование сокета в прослушиваемый сокет, т.е. такой,
 * на к-ром ядро принимает входящие сообщ-я от клиентов.
 * Второй пар-р задает макс кол-во клиентских соед-ий, к-рые ядро
 * ставит в очередь на прослушиваемом сокете.
 */
void Socket::Listen(size_t listenQueueSize) {
	if ((listen(m_listenFd, listenQueueSize) == -1)) {
		std::cerr << "SERVER Listen() failed: " << strerror(errno) << std::endl;
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
int Socket::Accept(struct sockaddr_in& client_addr) {
	socklen_t size = sizeof(client_addr);
	int clientFd = accept(m_listenFd, (struct sockaddr*)&client_addr, &size);
	if (clientFd == -1 && errno != EINTR) {
		std::cerr << "SERVER Accept() failed: " << strerror(errno) << std::endl;
		exit(EXIT_FAILURE);
	}
	//	Во время вып-я Accept пришел сигнал и прервал ее выполнение -> перезапускаем Accept
	else if (errno == EINTR) {
		return -1;
	}

	return clientFd;
}
