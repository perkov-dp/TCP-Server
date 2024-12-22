#include <variant>
#include <string>
#include <netinet/in.h>	//	sockaddr_in

enum class SocketType {
	SOCK_STREAM = __socket_type::SOCK_STREAM,
	SOCK_DGRAM = __socket_type::SOCK_DGRAM
};

class Endpoint {
public:
    Endpoint() = default;
    Endpoint(const std::string& addr, uint16_t port);
    Endpoint(const sockaddr_in& addr);
    Endpoint(sockaddr_in&& addr);
    Endpoint(const sockaddr_in6& addr);
    Endpoint(sockaddr_in6&& addr);
    Endpoint(const sockaddr& addr, socklen_t len);

    const std::variant<sockaddr_in, sockaddr_in6>& Addr() const;
    std::pair<const sockaddr*, int> RawAddr() const;
    bool operator == (const Endpoint& other) const;
    int Domain() const;

    std::string ToString() const;

private:
    std::variant<sockaddr_in, sockaddr_in6> m_addr = {};
};

class Socket {
public:
    Socket() = default;
    Socket(Endpoint&& endpoint, SocketType type);
    ~Socket();

    int Accept(struct sockaddr_in& client_addr);

private:
    void Bind();
	void Listen(size_t listenQueueSize);

private:
    int m_listenFd = 0;
    Endpoint m_address;
};
