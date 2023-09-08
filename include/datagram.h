#pragma once

#include <string>
#include <vector>

#include "pollable.h"
#include "io_buffer.h"

namespace loquat
{
    class Datagram : public ReadWritable
    {
        public:
            void Enqueue(std::string& to, std::vector<Byte>& data);
            virtual void OnRecv(std::string& from, std::vector<Byte>& data) = 0;

            void OnRead(int sock_fd) override;
            void OnWrite(int sock_fd) override;
        private:
            IOBuffer2 io_buffer_;
    };
}