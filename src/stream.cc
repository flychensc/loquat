#include <cassert>
#include <stdexcept>

#include <errno.h>
#include <sys/socket.h>

#include "stream.h"

namespace loquat
{
    using namespace std;

    void Stream::Send(vector<Byte>& data)
    {
        auto& outbuf = io_buffer_.write_queue_;

        while(!outbuf.empty())
        {
            auto& msg = outbuf.front();

            auto buf = msg.data()+io_buffer_.write_queue_head_offset_;
            auto len = msg.size()-io_buffer_.write_queue_head_offset_;

            auto written = send(sock_fd_, buf, len, 0);

            if (written < 0)
            {
                if (errno == EAGAIN)
                {
                    written = 0;
                    return;
                }
                else
                    throw runtime_error("Error writing to stream");
            }
            else if (written < len)
            {
                io_buffer_.write_queue_head_offset_ += written;
                return;
            }
            else
            {
                io_buffer_.write_queue_head_offset_ = 0;
            }

            outbuf.pop_front();
        }
    }

    void Stream::onRecv()
    {
        auto& inbuf = io_buffer_.read_buffer_;

        assert(io_buffer_.read_bytes_+io_buffer_.bytes_needed_ <= inbuf.size());

        auto buf = inbuf.data() + io_buffer_.read_bytes_;
        auto len = io_buffer_.bytes_needed_;

        auto bytes_in = recv(sock_fd_, buf, len, 0);
        if (bytes_in <= 0)
        {
            if (bytes_in == 0)
            {
                /* Socket is closed */
                throw runtime_error("Stream is closed");
            }

            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return;
            }

            throw runtime_error("Error reading from stream");
        }

        io_buffer_.read_bytes_ += bytes_in;
        io_buffer_.bytes_needed_ -= bytes_in;
    }
}