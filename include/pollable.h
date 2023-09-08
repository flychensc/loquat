#pragma once

namespace loquat
{
    class Pollable
    {
        public:
            virtual ~Pollable() = default;
    };

    class Acceptable : virtual public Pollable
    {
        public:
            virtual void OnAccept(int listen_sock) = 0;
    };

    class ReadWritable : virtual public Pollable
    {
        protected:
            virtual void OnRead(int sock_fd) = 0;
            virtual void OnWrite(int sock_fd) = 0;
            friend class Epoll;
    };

    class Closable : virtual public Pollable
    {
        public:
            virtual void OnClose(int sock_fd) = 0;
            friend class Epoll;
    };
}