#pragma once

namespace loquat
{
    class Pollable
    {
        public:
            virtual ~Pollable() = default;
    };

    class Acceptable : public Pollable
    {
        public:
            virtual void OnAccept(int listen_sock) = 0;
    };

    class Streamable : public Pollable
    {
        public:
            virtual void OnClose(int sock_fd) = 0;
        protected:
            virtual void OnRecv(int sock_fd) = 0;
            virtual void OnSend(int sock_fd) = 0;
            friend class Epoll;
    };

    class Unreliable : public Pollable
    {
        protected:
            virtual void OnSocketRecvfrom() = 0;
            virtual void OnSocketSendto() = 0;
            friend class Epoll;
    };
}