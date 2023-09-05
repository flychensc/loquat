#include <stdexcept>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "connector.h"

namespace loquat
{
    using namespace std;

    Connector::Connector(Epoll& poller) :
        epoll_(poller)
    {
        conn_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (conn_fd_ == -1)
            throw runtime_error("socket");

        epoll_.Join(conn_fd_,
            [this](int fd) -> void {
                onRecv();
            },
            [this](int fd) -> void {
                onSend();
            },
            nullptr);
    }

    Connector::~Connector()
    {
        epoll_.Leave(conn_fd_);
        ::close(conn_fd_);
    }

    void Connector::Bind(const string& ip4addr, int port)
    {
        struct sockaddr_in addr = {0};

        addr.sin_family = AF_INET;
        addr.sin_port = ::htons(port);

        if (::inet_pton(AF_INET, ip4addr.c_str(), &addr.sin_addr) == -1)
            throw runtime_error("inet_pton");

        if (::bind(conn_fd_, (struct sockaddr*)&addr, sizeof(struct sockaddr)) == -1)
            throw runtime_error("bind");
    }

    void Connector::Connect(const string& ip4addr, int port)
    {
        struct sockaddr_in addr = {0};

        addr.sin_family = AF_INET;
        addr.sin_port = ::htons(port);

        if (::inet_pton(AF_INET, ip4addr.c_str(), &addr.sin_addr) == -1)
            throw runtime_error("inet_pton");

        if (::connect(conn_fd_, (struct sockaddr*)&addr, sizeof(struct sockaddr)) == -1)
            throw runtime_error("connect");
    }
}