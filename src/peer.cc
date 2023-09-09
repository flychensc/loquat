#include <sstream>
#include <stdexcept>

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "peer.h"

namespace loquat
{
    using namespace std;

    Peer::Peer(int domain) :
        domain_(domain)
    {
        sock_fd_ = ::socket(domain_, SOCK_DGRAM | SOCK_NONBLOCK, 0);
        if (sock_fd_ == -1)
        {
            stringstream errinfo;
            errinfo << "socket:" << strerror(errno);
            throw runtime_error(errinfo.str());
        }
    }

    Peer::~Peer()
    {
        ::close(sock_fd_);
    }

    void Peer::Bind(const std::string& ipaddr, int port)
    {
        int optval = 1;
        socklen_t optlen = sizeof(optval);
        ::setsockopt(sock_fd_, SOL_SOCKET, SO_REUSEADDR, &optval, optlen);

        struct sockaddr_in addr = {0};

        addr.sin_family = domain_;
        addr.sin_port = ::htons(port);

        if (::inet_pton(domain_, ipaddr.c_str(), &addr.sin_addr) == -1)
        {
            stringstream errinfo;
            errinfo << "inet_pton:" << strerror(errno);
            throw runtime_error(errinfo.str());
        }

        if (::bind(sock_fd_, (struct sockaddr*)&addr, sizeof(addr)) == -1)
        {
            stringstream errinfo;
            errinfo << "bind:" << strerror(errno);
            throw runtime_error(errinfo.str());
        }
    }

    void Peer::Bind(const std::string& unix_path)
    {
        int optval = 1;
        socklen_t optlen = sizeof(optval);
        ::setsockopt(sock_fd_, SOL_SOCKET, SO_REUSEADDR, &optval, optlen);

        struct sockaddr_un addr = {0};

        addr.sun_family = domain_;
        ::strcpy(addr.sun_path, unix_path.c_str());

        unlink(unix_path.c_str());

        if (::bind(sock_fd_, (struct sockaddr*)&addr, sizeof(addr)) == -1)
        {
            stringstream errinfo;
            errinfo << "bind:" << strerror(errno);
            throw runtime_error(errinfo.str());
        }
    }

    void Peer::Enqueue(const std::string& to_ip, int port, std::vector<Byte>& data)
    {
        struct sockaddr toaddr;
        socklen_t addrlen;

        if (AF_INET == domain_)
        {
            struct sockaddr_in *addr = (struct sockaddr_in *)&toaddr;
            addrlen = sizeof(struct sockaddr_in);

            addr->sin_family = domain_;
            addr->sin_port = ::htons(port);

            if (::inet_pton(domain_, to_ip.c_str(), &addr->sin_addr) == -1)
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

            if (::inet_pton(domain_, to_ip.c_str(), &addr->sin6_addr) == -1)
            {
                stringstream errinfo;
                errinfo << "inet_pton:" << strerror(errno);
                throw runtime_error(errinfo.str());
            }
        }

        Datagram::Enqueue(toaddr, addrlen, data);
    }

    void Peer::Enqueue(const std::string& to_path, std::vector<Byte>& data)
    {
        struct sockaddr toaddr;
        struct sockaddr_un *addr = (struct sockaddr_un *)&toaddr;
        socklen_t addrlen = sizeof(struct sockaddr_un);

        addr->sun_family = domain_;
        ::strcpy(addr->sun_path, to_path.c_str());

        Datagram::Enqueue(toaddr, addrlen, data);
    }
}