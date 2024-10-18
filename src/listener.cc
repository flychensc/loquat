#include <sstream>
#include <stdexcept>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
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
        if (sock_fd_ == -1)
        {
            stringstream errinfo;
            errinfo << "accept:" << strerror(errno);
            throw runtime_error(errinfo.str());
        }

        /*2.set non-block*/
        ::fcntl(sock_fd_, F_SETFL, ::fcntl(sock_fd_, F_GETFL) | O_NONBLOCK);
    }

    Connection::~Connection()
    {
        ::close(sock_fd_);
    }

    Listener::Listener(int domain, int max_connections) : domain_(domain),
                                                          backlog_(max_connections)
    {
        listen_fd_ = ::socket(domain_, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (listen_fd_ == -1)
        {
            stringstream errinfo;
            errinfo << "socket:" << strerror(errno);
            throw runtime_error(errinfo.str());
        }
    }

    Listener::~Listener()
    {
        ::close(listen_fd_);
    }

    void Listener::Listen(const string &ipaddr, int port)
    {
        int optval = 1;
        socklen_t optlen = sizeof(optval);
        ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &optval, optlen);

        struct sockaddr toaddr;
        socklen_t addrlen;

        if (AF_INET == domain_)
        {
            struct sockaddr_in *addr = (struct sockaddr_in *)&toaddr;
            addrlen = sizeof(struct sockaddr_in);

            addr->sin_family = domain_;
            addr->sin_port = ::htons(port);

            if (::inet_pton(domain_, ipaddr.c_str(), &addr->sin_addr) != 1)
            {
                stringstream errinfo;
                errinfo << "inet_pton:" << strerror(errno);
                throw runtime_error(errinfo.str());
            }
        }
        else if (AF_INET6 == domain_)
        {
            struct sockaddr_in6 *addr = (struct sockaddr_in6 *)&toaddr;
            addrlen = sizeof(struct sockaddr_in6);

            addr->sin6_family = domain_;
            addr->sin6_port = ::htons(port);

            if (::inet_pton(domain_, ipaddr.c_str(), &addr->sin6_addr) != 1)
            {
                stringstream errinfo;
                errinfo << "inet_pton:" << strerror(errno);
                throw runtime_error(errinfo.str());
            }
        }

        if (::bind(listen_fd_, &toaddr, addrlen) == -1)
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
}