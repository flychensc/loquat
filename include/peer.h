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

        Peer(const Peer &) = delete;
        Peer(Peer &&) = delete;

        /** @brief get sock id
         * @return sock id
         */
        int Sock() { return sock_fd_; };

        /** @brief binding a ipv4/ipv6 address+port
         * @param ipaddr ip address
         * @param port port number
         */
        void Bind(const std::string &ipaddr, int port);

        /** @brief binding a unix path
         * @param unix_path unix path
         */
        void Bind(const std::string &unix_path);

        void Enqueue(const SockAddr &toaddr, const std::vector<Byte> &data);

        /** @brief enqueue output data
         * @param ipaddr destination ip
         * @param port destination port
         * @param data output data
         */
        void Enqueue(const std::string &to_ip, int port, const std::vector<Byte> &data);

        /** @brief enqueue output data
         * @param to_path unix path
         * @param data output data
         */
        void Enqueue(const std::string &to_path, const std::vector<Byte> &data);

    protected:
        void OnWrite(int sock_fd) override;

    private:
        void SetReadReady();
        void ClearReadReady();
        void SetWriteReady();
        void ClearWriteReady();

        int domain_;
        int sock_fd_;
    };
}