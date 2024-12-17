#include <sstream>
#include <stdexcept>

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "connector.h"
#include "epoll.h"

namespace loquat
{
    using namespace std;

    Connector::Connector(Stream::Type type, int domain) : Stream(type),
                                                          domain_(domain),
                                                          connect_flag_(false)
    {
        sock_fd_ = ::socket(domain_, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        if (sock_fd_ == -1)
        {
            stringstream errinfo;
            errinfo << "socket:" << strerror(errno);
            throw runtime_error(errinfo.str());
        }
        spdlog::debug("Connector:{}", sock_fd_);
    }

    Connector::~Connector()
    {
        ::close(sock_fd_);
        spdlog::debug("~Connector:{}", sock_fd_);
    }

    void Connector::Bind(const string &ipaddr, int port)
    {
        int optval = 1;
        socklen_t optlen = sizeof(optval);
        ::setsockopt(sock_fd_, SOL_SOCKET, SO_REUSEADDR, &optval, optlen);

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

        if (::bind(sock_fd_, toaddr, addrlen) == -1)
        {
            stringstream errinfo;
            errinfo << "bind:" << strerror(errno);
            throw runtime_error(errinfo.str());
        }
    }

    void Connector::Bind(const string &unix_path)
    {
        int optval = 1;
        socklen_t optlen = sizeof(optval);
        ::setsockopt(sock_fd_, SOL_SOCKET, SO_REUSEADDR, &optval, optlen);

        struct sockaddr_un addr = {0};

        addr.sun_family = domain_;
        ::strcpy(addr.sun_path, unix_path.c_str());

        unlink(unix_path.c_str());

        if (::bind(sock_fd_, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        {
            stringstream errinfo;
            errinfo << "bind:" << strerror(errno);
            throw runtime_error(errinfo.str());
        }
    }

    void Connector::Connect(const string &ipaddr, int port)
    {
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

        if (::connect(sock_fd_, toaddr, addrlen) == -1)
        {
            if (errno != EINPROGRESS)
            {
                stringstream errinfo;
                errinfo << "connect:" << strerror(errno);
                throw runtime_error(errinfo.str());
            }
        }
    }

    void Connector::Connect(const string &unix_path)
    {
        struct sockaddr_un addr = {0};

        addr.sun_family = domain_;
        ::strcpy(addr.sun_path, unix_path.c_str());

        if (::connect(sock_fd_, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        {
            if (errno != EINPROGRESS)
            {
                stringstream errinfo;
                errinfo << "connect:" << strerror(errno);
                throw runtime_error(errinfo.str());
            }
        }
    }

    void Connector::Enqueue(const std::vector<Byte> &data)
    {
        Stream::Enqueue(data);
        if (PktsEnqueued() > 0)
            SetWriteReady();
    }

    void Connector::OnRead(int sock_fd)
    {
        if (connect_flag_)
        {
            Stream::OnRead(sock_fd);
        }
        else
        {
            int optval;
            socklen_t optlen = sizeof(optval);

            if (::getsockopt(sock_fd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
            {
                stringstream errinfo;
                errinfo << "getsockopt:" << strerror(errno);
                throw runtime_error(errinfo.str());
            }
            if (optval != 0)
            {
                stringstream errinfo;
                errinfo << "connect:" << strerror(errno);
                throw runtime_error(errinfo.str());
            }

            /*connected*/
            connect_flag_ = true;
        }
    }

    void Connector::OnWrite(int sock_fd)
    {
        Stream::OnWrite(sock_fd);
        if (PktsEnqueued() == 0)
            ClearWriteReady();
    }

    void Connector::SetWriteReady()
    {
        Epoll::GetInstance()->DataOutReady(Sock());
    }

    void Connector::ClearWriteReady()
    {
        Epoll::GetInstance()->DataOutClear(Sock());
    }
}