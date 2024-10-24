#include "io_buffer.h"

#include "gtest/gtest.h"
namespace
{
    TEST(IOBuffer, constructor)
    {
        loquat::IOBuffer io_buffer;

        EXPECT_EQ(io_buffer.read_bytes_, 0);
        EXPECT_EQ(io_buffer.bytes_needed_, 0);
        EXPECT_EQ(io_buffer.read_buffer_.size(), loquat::IOBuffer::kReadBufferSize);

        EXPECT_EQ(io_buffer.write_queue_head_offset_, 0);
        EXPECT_EQ(io_buffer.write_queue_.size(), 0);
    }
    TEST(IOBuffer2, constructor)
    {
        loquat::IOBuffer io_buffer;

        EXPECT_EQ(io_buffer.read_bytes_, 0);
        EXPECT_EQ(io_buffer.read_buffer_.size(), loquat::IOBuffer::kReadBufferSize);

        EXPECT_EQ(io_buffer.write_queue_.size(), 0);
    }
}
