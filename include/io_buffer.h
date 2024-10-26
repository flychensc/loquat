#pragma once

#include <cinttypes>
#include <list>
#include <tuple>
#include <vector>

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>

namespace loquat
{
    using Byte = uint8_t;

    struct SockAddr
    {
        union
        {
            struct sockaddr sa;
            struct sockaddr_in v4;
            struct sockaddr_in6 v6;
            struct sockaddr_un un;
        } addr;

        socklen_t addrlen;

        SockAddr() : addrlen(0)
        {
            memset(this, 0, sizeof(*this));
        }
    };

    struct IOBuffer
    {
        static constexpr int kReadBufferSize = 2 * 1024;

        IOBuffer() : read_bytes_(0),
                     bytes_needed_(0),
                     write_queue_head_offset_(0)
        {
            read_buffer_.resize(kReadBufferSize);
        };

        /* Read buffer */
        std::vector<Byte> read_buffer_;
        /* Number of bytes currently in read buffer */
        std::size_t read_bytes_;
        /* Num bytes needed for next process step */
        std::size_t bytes_needed_;

        /* Write queue */
        std::list<std::vector<Byte>> write_queue_;
        /* Bytes already sent out from head of write_queue */
        std::size_t write_queue_head_offset_;
    };

    struct IOBuffer2
    {
        static constexpr int kReadBufferSize = 2 * 1024;

        IOBuffer2() : read_bytes_(0)
        {
            read_buffer_.resize(kReadBufferSize);
        };

        /* Read buffer */
        std::vector<Byte> read_buffer_;
        /* Number of bytes currently in read buffer */
        std::size_t read_bytes_;

        /* Write queue */
        std::list<std::tuple<SockAddr, std::vector<Byte>>> write_queue_;
    };
}
