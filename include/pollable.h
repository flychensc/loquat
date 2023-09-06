#pragma once

#include <functional>

namespace loquat
{
    class Pollable
    {
        public:
            // handle accept new TCP connection
            virtual void OnSocketAccept(int conn_sock) = 0;
            // handle socket close event
            virtual void OnSocketClose(int sock_fd) = 0;
            // handle socket readable (recv)
            virtual void OnSocketRecv(int sock_fd) = 0;
            // handle socket writeable (send)
            virtual void OnSocketSend(int sock_fd) = 0;
            // handle socket readable (recvfrom)
            virtual void OnSocketRecvfrom(int sock_fd) = 0;
            // handle socket writeable (sendto)
            virtual void OnSocketSendto(int sock_fd) = 0;
    };
}