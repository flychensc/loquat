#include <stdexcept>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "listener.h"

namespace loquat
{
    using namespace std;

    Listener::Listener(Epoll& poller, int max_connections) :
        backlog_(max_connections),
        epoll_(poller)
    {
        listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (listen_fd_ == -1)
            throw runtime_error("socket");

        epoll_.Join(listen_fd_,
            [this](int fd) -> void {
                onConnect(fd);
            });
    }

    Listener::~Listener()
    {
        for(auto&connection : connections_)
        {
            epoll_.Leave(connection.first);
        }
        epoll_.Leave(listen_fd_);
        ::close(listen_fd_);
    }

    void Listener::Listen(const string& ip4addr, int port)
    {
        struct sockaddr_in addr = {0};

        addr.sin_family = AF_INET;
        addr.sin_port = ::htons(port);

        if (::inet_pton(AF_INET, ip4addr.c_str(), &addr.sin_addr) == -1)
            throw runtime_error("inet_pton");

        if (::bind(listen_fd_, (struct sockaddr*)&addr, sizeof(struct sockaddr)) == -1)
            throw runtime_error("bind");

        if (::listen(listen_fd_, backlog_) == -1)
            throw runtime_error("bind");

        epoll_.Wait();
    }

    void Listener::onConnect(int fd)
    {
        connections_.insert({fd, Stream(fd)});

        if (connect_callback_ != nullptr)
        {
            connect_callback_(connections_.at(fd));
        }

        epoll_.Join(fd,
            [this](int fd) -> void {
                connections_.at(fd).onRecv();
            },
            [this](int fd) -> void {
                connections_.at(fd).onSend();
            },
            [this](int fd) -> void {
                onDisconnect(fd);
            });
    }

    void Listener::onDisconnect(int fd)
    {
        if (disconnect_callback_ != nullptr)
        {
            disconnect_callback_(connections_.at(fd));
        }

        connections_.erase(fd);
    }
}