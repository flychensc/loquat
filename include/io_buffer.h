#pragma once

#include <cinttypes>
#include <list>
#include <vector>

namespace loquat
{
    using Byte = uint8_t;

    struct IOBuffer
    {
        static constexpr int kReadBufferSize = 2*1024;

        IOBuffer() : read_any_(true),
            read_bytes_(0),
            bytes_needed_(kReadBufferSize),
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
        bool read_any_;

        /* Write queue */
        std::list<std::vector<Byte>> write_queue_;
        /* Bytes already sent out from head of write_queue */
        std::size_t write_queue_head_offset_;
    };
}