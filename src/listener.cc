#include <sstream>
#include <stdexcept>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <string.h>
#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "listener.h"
#include "epoll.h"

namespace loquat
{
    using namespace std;

    Connection::Connection(Stream::Type type, int listen_fd) : Stream(type)
    {
        SockAddr addr;
        socklen_t addrlen = sizeof(addr);

        /*1.accept new connection*/
        sock_fd_ = ::accept(listen_fd, &addr.addr.sa, &addrlen);
        if (sock_fd_ == -1)
        {
            stringstream errinfo;
            errinfo << "accept:" << strerror(errno);
            throw runtime_error(errinfo.str());
        }

        /*2.set non-block*/
        ::fcntl(sock_fd_, F_SETFL, ::fcntl(sock_fd_, F_GETFL) | O_NONBLOCK);
        // set close-on-exec
        ::fcntl(sock_fd_, F_SETFD, FD_CLOEXEC);

        spdlog::debug("Connection:{}", sock_fd_);
    }

    Connection::~Connection()
    {
        ::close(sock_fd_);

        spdlog::debug("~Connection:{}", sock_fd_);
    }

    Listener::Listener(int domain, int max_connections) : domain_(domain),
                                                          backlog_(max_connections)
    {
        listen_fd_ = ::socket(domain_, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        if (listen_fd_ == -1)
        {
            stringstream errinfo;
            errinfo << "socket:" << strerror(errno);
            throw runtime_error(errinfo.str());
        }

        spdlog::debug("Listener:{}", listen_fd_);
    }

    Listener::~Listener()
    {
        ::close(listen_fd_);

        spdlog::debug("~Listener:{}", listen_fd_);
    }

    void Listener::Listen(const string &ipaddr, int port)
    {
        int optval = 1;
        socklen_t optlen = sizeof(optval);
        ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &optval, optlen);

        struct sockaddr_in addr4;
        struct sockaddr_in6 addr6;
        struct sockaddr *toaddr;
        socklen_t addrlen;

        if (AF_INET == domain_)
        {
            toaddr = (struct sockaddr *)&addr4;
            addrlen = sizeof(struct sockaddr_in);

            addr4.sin_family = domain_;
            addr4.sin_port = ::htons(port);

            if (::inet_pton(domain_, ipaddr.c_str(), &addr4.sin_addr) != 1)
            {
                stringstream errinfo;
                errinfo << "inet_pton:" << strerror(errno);
                throw runtime_error(errinfo.str());
            }
        }
        else if (AF_INET6 == domain_)
        {
            toaddr = (struct sockaddr *)&addr6;
            addrlen = sizeof(struct sockaddr_in6);

            addr6.sin6_family = domain_;
            addr6.sin6_port = ::htons(port);

            if (::inet_pton(domain_, ipaddr.c_str(), &addr6.sin6_addr) != 1)
            {
                stringstream errinfo;
                errinfo << "inet_pton:" << strerror(errno);
                throw runtime_error(errinfo.str());
            }
        }

        if (::bind(listen_fd_, toaddr, addrlen) == -1)
        {
            stringstream errinfo;
            errinfo << "bind:" << strerror(errno);
            throw runtime_error(errinfo.str());
        }

        if (::listen(listen_fd_, backlog_) == -1)
        {
            stringstream errinfo;
            errinfo << "listen:" << strerror(errno);
            throw runtime_error(errinfo.str());
        }
    }

    void Listener::Listen(const string &unix_path)
    {
        int optval = 1;
        socklen_t optlen = sizeof(optval);
        ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &optval, optlen);

        struct sockaddr_un addr = {0};

        addr.sun_family = domain_;
        ::strcpy(addr.sun_path, unix_path.c_str());

        unlink(unix_path.c_str());

        if (::bind(listen_fd_, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        {
            stringstream errinfo;
            errinfo << "bind:" << strerror(errno);
            throw runtime_error(errinfo.str());
        }

        if (::listen(listen_fd_, backlog_) == -1)
        {
            stringstream errinfo;
            errinfo << "listen:" << strerror(errno);
            throw runtime_error(errinfo.str());
        }
    }

    void Connection::Enqueue(const std::vector<Byte> &data)
    {
        Stream::Enqueue(data);
        if (PktsEnqueued() > 0)
            SetWriteReady();
    }

    void Connection::OnWrite(int sock_fd)
    {
        Stream::OnWrite(sock_fd);
        if (PktsEnqueued() == 0)
            ClearWriteReady();
    }

    void Connection::SetWriteReady()
    {
        Epoll::GetInstance()->DataOutReady(Sock());
    }

    void Connection::ClearWriteReady()
    {
        Epoll::GetInstance()->DataOutClear(Sock());
    }
}