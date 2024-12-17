#pragma once

#include <string>

#include <sys/socket.h>

#include "pollable.h"
#include "stream.h"

namespace loquat
{
    class Connection : public Stream
    {
    public:
        Connection(Stream::Type type, int listen_fd);
        Connection(int listen_fd) : Connection(Stream::Type::Unframed, listen_fd) {}
        ~Connection();

        Connection(const Connection &) = delete;
        Connection(Connection &&) = delete;

        /** @brief get sock id
         * @return sock id
         */
        int Sock() { return sock_fd_; };

        /** @brief enqueue output data
         * @param data output data
         */
        void Enqueue(const std::vector<Byte> &data);

    protected:
        void OnWrite(int sock_fd) override;

    private:
        void SetReadReady();
        void ClearReadReady();
        void SetWriteReady();
        void ClearWriteReady();

        int sock_fd_;
    };

    class Listener : public Acceptable
    {
    public:
        Listener(int domain, int max_connections);
        Listener(int max_connections) : Listener(AF_INET, max_connections) {};
        ~Listener();

        Listener(const Listener &) = delete;
        Listener(Listener &&) = delete;

        /** @brief get sock id
         * @return sock id
         */
        int Sock() { return listen_fd_; };

        /** @brief listen on a ipv4/ipv6 address+port
         * @param ipaddr ip address
         * @param port port number
         */
        void Listen(const std::string &ipaddr, int port);
        /** @brief listen on a unix path
         * @param unix_path unix path
         */
        void Listen(const std::string &unix_path);

    private:
        int domain_;
        int listen_fd_;
        int backlog_;
    };
}