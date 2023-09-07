#include <stdexcept>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "listener.h"

namespace loquat
{
    using namespace std;

    Connection::Connection(int listen_fd)
    {
        struct sockaddr_in addr = {0};
        socklen_t addrlen = sizeof(struct sockaddr_in);

        /*1.accept new connection*/
        sock_fd_ = ::accept(listen_fd, (struct sockaddr *)&addr, &addrlen);
        if(sock_fd_ == -1)
            throw runtime_error("accept");

        /*2.set non-block*/
        ::fcntl(sock_fd_, F_SETFL, ::fcntl(sock_fd_, F_GETFL) | O_NONBLOCK);
    }

    Connection::~Connection()
    {
        ::close(sock_fd_);
    }

    Listener::Listener(int max_connections) :
        backlog_(max_connections)
    {
        listen_fd_ = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (listen_fd_ == -1)
            throw runtime_error("socket");
    }

    Listener::~Listener()
    {
        ::close(listen_fd_);
    }

    void Listener::Listen(const string& ip4addr, int port)
    {
        int optval = 1;
        socklen_t optlen = sizeof(optval);
        if (::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &optval, optlen) == -1)
            throw runtime_error("setsockopt");

        struct sockaddr_in addr = {0};

        addr.sin_family = AF_INET;
        addr.sin_port = ::htons(port);

        if (::inet_pton(AF_INET, ip4addr.c_str(), &addr.sin_addr) == -1)
            throw runtime_error("inet_pton");

        if (::bind(listen_fd_, (struct sockaddr*)&addr, sizeof(struct sockaddr)) == -1)
            throw runtime_error("bind");

        if (::listen(listen_fd_, backlog_) == -1)
            throw runtime_error("bind");
    }
}