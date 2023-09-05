#pragma once

#include <string>

#include "epoll.h"
#include "stream.h"

namespace loquat
{
    class Connector : public Stream
    {
        public:
            Connector(Epoll& poller);
            ~Connector();

            Connector( const Connector& ) = delete;
            Connector( Connector&& ) = delete;

            void Bind(const std::string& ip4addr, int port);
            void Connect(const std::string& ip4addr, int port);
        private:
            Epoll& epoll_;
    };
}