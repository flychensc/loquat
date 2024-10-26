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
        void Enqueue(const SockAddr &toaddr, const std::vector<Byte> &data);

    protected:
        virtual void OnRecv(const SockAddr &fromaddr, const std::vector<Byte> &data) = 0;

        void OnRead(int sock_fd) override;
        void OnWrite(int sock_fd) override;

    private:
        IOBuffer2 io_buffer_;
        std::mutex mutex_;
    };
}