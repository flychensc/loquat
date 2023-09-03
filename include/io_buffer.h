#pragma once

#include <cinttypes>
#include <deque>
#include <vector>

namespace loquat
{
    using Byte = uint8_t;

    class IOBuffer
    {
        static constexpr int kReadBufferSize = 2*1024;
        public:
        private:
            /* Read buffer */
            std::vector<Byte> read_buffer_;
            /* Number of bytes currently in read buffer */
            std::size_t read_bytes_;
            /* Num bytes needed for next process step */
            std::size_t bytes_needed_;

            /* Write queue */
            std::deque<std::vector<Byte>> write_queue_;
            /* Bytes already sent out from head of write_queue */
            std::size_t write_queue_head_offset_;
    };
}