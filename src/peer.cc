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

#include "peer.h"
#include "epoll.h"

namespace loquat
{
    using namespace std;

    Peer::Peer(int domain) : domain_(domain)
    {
        sock_fd_ = ::socket(domain_, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        if (sock_fd_ == -1)
        {
            stringstream errinfo;
            errinfo << "socket:" << strerror(errno);
            throw runtime_error(errinfo.str());
        }

        spdlog::debug("Peer:{}", sock_fd_);
    }

    Peer::~Peer()
    {
        ::close(sock_fd_);

        spdlog::debug("~Peer:{}", sock_fd_);
    }

    void Peer::Bind(const std::string &ipaddr, int port)
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

    void Peer::Bind(const std::string &unix_path)
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

    void Peer::Enqueue(const SockAddr &toaddr, const std::vector<Byte> &data)
    {
        Datagram::Enqueue(toaddr, data);
        if (PktsEnqueued() > 0)
            SetWriteReady();
    }

    void Peer::Enqueue(const std::string &to_ip, int port, const std::vector<Byte> &data)
    {
        SockAddr toaddr;

        if (AF_INET == domain_)
        {
            toaddr.addrlen = sizeof(struct sockaddr_in);

            toaddr.addr.v4.sin_family = domain_;
            toaddr.addr.v4.sin_port = ::htons(port);

            if (::inet_pton(domain_, to_ip.c_str(), &toaddr.addr.v4.sin_addr) != 1)
            {
                stringstream errinfo;
                errinfo << "inet_pton:" << strerror(errno);
                throw runtime_error(errinfo.str());
            }
        }
        else if (AF_INET6 == domain_)
        {
            toaddr.addrlen = sizeof(struct sockaddr_in6);

            toaddr.addr.v6.sin6_family = domain_;
            toaddr.addr.v6.sin6_port = ::htons(port);

            if (::inet_pton(domain_, to_ip.c_str(), &toaddr.addr.v6.sin6_addr) != 1)
            {
                stringstream errinfo;
                errinfo << "inet_pton:" << strerror(errno);
                throw runtime_error(errinfo.str());
            }
        }

        Datagram::Enqueue(toaddr, data);
    }

    void Peer::Enqueue(const std::string &to_path, const std::vector<Byte> &data)
    {
        SockAddr toaddr;
        toaddr.addrlen = sizeof(struct sockaddr_un);

        toaddr.addr.un.sun_family = domain_;
        ::strcpy(toaddr.addr.un.sun_path, to_path.c_str());

        Datagram::Enqueue(toaddr, data);
    }

    void Peer::OnWrite(int sock_fd)
    {
        Datagram::OnWrite(sock_fd);
        if (PktsEnqueued() == 0)
            ClearWriteReady();
    }

    void Peer::SetWriteReady()
    {
        Epoll::GetInstance()->DataOutReady(Sock());
    }

    void Peer::ClearWriteReady()
    {
        Epoll::GetInstance()->DataOutClear(Sock());
    }
}