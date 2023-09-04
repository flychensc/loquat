#pragma once

#include <string>

#include "epoll.h"

namespace loquat
{
    class Listener
    {
        public:
            Listener(Epoll& poller);
            ~Listener();

            Listener( const Listener& ) = delete;
            Listener( Listener&& ) = delete;

            void Listen(const std::string& ip4addr, int port);
        private:
            int listen_fd_;
            Epoll& epoll_;
    };
}