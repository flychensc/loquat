#include <stdexcept>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "connector.h"

namespace loquat
{
    using namespace std;

    Connector::Connector() : connect_flag_(false)
    {
        sock_fd_ = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (sock_fd_ == -1)
            throw runtime_error("socket");
    }

    Connector::~Connector()
    {
        ::close(sock_fd_);
    }

    void Connector::Bind(const string& ip4addr, int port)
    {
        struct sockaddr_in addr = {0};

        addr.sin_family = AF_INET;
        addr.sin_port = ::htons(port);

        if (::inet_pton(AF_INET, ip4addr.c_str(), &addr.sin_addr) == -1)
            throw runtime_error("inet_pton");

        if (::bind(sock_fd_, (struct sockaddr*)&addr, sizeof(struct sockaddr)) == -1)
            throw runtime_error("bind");
    }

    void Connector::Connect(const string& ip4addr, int port)
    {
        struct sockaddr_in addr = {0};

        addr.sin_family = AF_INET;
        addr.sin_port = ::htons(port);

        if (::inet_pton(AF_INET, ip4addr.c_str(), &addr.sin_addr) == -1)
            throw runtime_error("inet_pton");

        if (::connect(sock_fd_, (struct sockaddr*)&addr, sizeof(struct sockaddr)) == -1)
        {
            if (errno != EINPROGRESS)
                throw runtime_error("connect");
        }
    }

    void Connector::OnRecv(int sock_fd)
    {
        if (connect_flag_)
        {
            Stream::OnRecv(sock_fd);
        }
        else
        {
            int optval;
            socklen_t optlen = sizeof(optval);

            if (::getsockopt(sock_fd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
                throw runtime_error("getsockopt");
            if (optval != 0)
                throw runtime_error("connect error");

            /*connected*/
            connect_flag_ = true;
        }
    }
}