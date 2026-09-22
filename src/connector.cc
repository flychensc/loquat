#include <cstring>
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

    Connector::Connector(Stream::Type type, int domain) : Stream(type),
                                                          domain_(domain),
                                                          connected_(false)
    {
        sock_fd_ = ::socket(domain_, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        if (sock_fd_ == -1)
        {
            std::stringstream errinfo;
            errinfo << "socket:" << strerror(errno);
            throw std::runtime_error(errinfo.str());
        }
        spdlog::debug("Connector:{}", sock_fd_);
    }

    Connector::~Connector()
    {
        ::close(sock_fd_);
        spdlog::debug("~Connector:{}", sock_fd_);
    }

    void Connector::Bind(const std::string &ipaddr, int port)
    {
        int optval = 1;
        socklen_t optlen = sizeof(optval);
        ::setsockopt(sock_fd_, SOL_SOCKET, SO_REUSEADDR, &optval, optlen);

        struct sockaddr_in addr4;
        struct sockaddr_in6 addr6;
        struct sockaddr *toaddr = nullptr;
        socklen_t addrlen = 0;

        if (AF_INET == domain_)
        {
            toaddr = (struct sockaddr *)&addr4;
            addrlen = sizeof(struct sockaddr_in);

            addr4.sin_family = static_cast<sa_family_t>(domain_);
            addr4.sin_port = ::htons(static_cast<uint16_t>(port));

            if (::inet_pton(domain_, ipaddr.c_str(), &addr4.sin_addr) != 1)
            {
                std::stringstream errinfo;
                errinfo << "inet_pton:" << strerror(errno);
                throw std::runtime_error(errinfo.str());
            }
        }
        else if (AF_INET6 == domain_)
        {
            toaddr = (struct sockaddr *)&addr6;
            addrlen = sizeof(struct sockaddr_in6);

            addr6.sin6_family = static_cast<sa_family_t>(domain_);
            addr6.sin6_port = ::htons(static_cast<uint16_t>(port));

            if (::inet_pton(domain_, ipaddr.c_str(), &addr6.sin6_addr) != 1)
            {
                std::stringstream errinfo;
                errinfo << "inet_pton:" << strerror(errno);
                throw std::runtime_error(errinfo.str());
            }
        }

        if (::bind(sock_fd_, toaddr, addrlen) == -1)
        {
            std::stringstream errinfo;
            errinfo << "bind:" << strerror(errno);
            throw std::runtime_error(errinfo.str());
        }
    }

    void Connector::Bind(const std::string &unix_path)
    {
        int optval = 1;
        socklen_t optlen = sizeof(optval);
        ::setsockopt(sock_fd_, SOL_SOCKET, SO_REUSEADDR, &optval, optlen);

        struct sockaddr_un addr = {};

        addr.sun_family = static_cast<sa_family_t>(domain_);
        std::strncpy(addr.sun_path, unix_path.c_str(), sizeof(addr.sun_path) - 1);
        addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

        unlink(unix_path.c_str());

        if (::bind(sock_fd_, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        {
            std::stringstream errinfo;
            errinfo << "bind:" << strerror(errno);
            throw std::runtime_error(errinfo.str());
        }
    }

    void Connector::Connect(const std::string &ipaddr, int port)
    {
        struct sockaddr_in addr4;
        struct sockaddr_in6 addr6;
        struct sockaddr *toaddr = nullptr;
        socklen_t addrlen = 0;

        if (AF_INET == domain_)
        {
            toaddr = (struct sockaddr *)&addr4;
            addrlen = sizeof(struct sockaddr_in);

            addr4.sin_family = static_cast<sa_family_t>(domain_);
            addr4.sin_port = ::htons(static_cast<uint16_t>(port));

            if (::inet_pton(domain_, ipaddr.c_str(), &addr4.sin_addr) != 1)
            {
                std::stringstream errinfo;
                errinfo << "inet_pton:" << strerror(errno);
                throw std::runtime_error(errinfo.str());
            }
        }
        else if (AF_INET6 == domain_)
        {
            toaddr = (struct sockaddr *)&addr6;
            addrlen = sizeof(struct sockaddr_in6);

            addr6.sin6_family = static_cast<sa_family_t>(domain_);
            addr6.sin6_port = ::htons(static_cast<uint16_t>(port));

            if (::inet_pton(domain_, ipaddr.c_str(), &addr6.sin6_addr) != 1)
            {
                std::stringstream errinfo;
                errinfo << "inet_pton:" << strerror(errno);
                throw std::runtime_error(errinfo.str());
            }
        }

        if (::connect(sock_fd_, toaddr, addrlen) == -1)
        {
            if (errno != EINPROGRESS)
            {
                std::stringstream errinfo;
                errinfo << "connect:" << strerror(errno);
                throw std::runtime_error(errinfo.str());
            }
        }
    }

    void Connector::Connect(const std::string &unix_path)
    {
        struct sockaddr_un addr = {};

        addr.sun_family = static_cast<sa_family_t>(domain_);
        ::strcpy(addr.sun_path, unix_path.c_str());

        if (::connect(sock_fd_, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        {
            if (errno != EINPROGRESS)
            {
                std::stringstream errinfo;
                errinfo << "connect:" << strerror(errno);
                throw std::runtime_error(errinfo.str());
            }
        }
    }

    void Connector::Enqueue(std::vector<Byte> data)
    {
        Stream::Enqueue(std::move(data));
        if (PktsEnqueued() > 0)
            SetWriteReady();
    }

    void Connector::OnWrite(int sock_fd)
    {
        if (!connected_)
        {
            // 非阻塞 connect 完成时会触发 EPOLLOUT，此时检查 SO_ERROR 确认连接
            int optval = 0;
            socklen_t optlen = sizeof(optval);
            if (::getsockopt(sock_fd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
            {
                std::ostringstream errinfo;
                errinfo << "getsockopt:" << strerror(errno);
                throw std::runtime_error(errinfo.str());
            }
            if (optval != 0)
            {
                std::ostringstream errinfo;
                errinfo << "connect failed:" << strerror(optval);
                throw std::runtime_error(errinfo.str());
            }
            connected_ = true;
            spdlog::debug("Connector connected:{}", sock_fd);
        }

        Stream::OnWrite(sock_fd);
        if (PktsEnqueued() == 0)
            ClearWriteReady();
    }

    void Connector::SetWriteReady()
    {
        if (auto e = epoll())
            e->DataOutReady(Sock());
    }

    void Connector::ClearWriteReady()
    {
        if (auto e = epoll())
            e->DataOutClear(Sock());
    }
}