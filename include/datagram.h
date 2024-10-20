#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "pollable.h"
#include "io_buffer.h"

namespace loquat
{
    class Datagram : public ReadWritable
    {
    public:
        void Enqueue(struct sockaddr &toaddr, socklen_t addrlen, const std::vector<Byte> &data);
        virtual void OnRecv(struct sockaddr &fromaddr, socklen_t addrlen, std::vector<Byte> &data) = 0;

        void OnRead(int sock_fd) override;
        void OnWrite(int sock_fd) override;

    private:
        IOBuffer2 io_buffer_;
        std::mutex mutex_;
    };
}