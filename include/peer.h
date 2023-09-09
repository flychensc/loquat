#pragma once

#include <string>
#include <vector>

#include "datagram.h"

namespace loquat
{
    class Peer : public Datagram
    {
        public:
            Peer(int domain);
            Peer() : Peer(AF_INET) {};
            ~Peer();

            Peer( const Peer& ) = delete;
            Peer( Peer&& ) = delete;

            int Sock() { return sock_fd_; };

            void Bind(const std::string& ipaddr, int port);
            void Bind(const std::string& unix_path);

            void Enqueue(const std::string& to_ip, int port, const std::vector<Byte>& data);
            void Enqueue(const std::string& to_path, const std::vector<Byte>& data);
        private:
            int domain_;
            int sock_fd_;
    };
}