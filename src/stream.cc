#include <cassert>
#include <stdexcept>

#include <errno.h>
#include <sys/socket.h>

#include "stream.h"

namespace loquat
{
    using namespace std;

    void Stream::Enqueue(vector<Byte>& data)
    {
        auto& outbuf = io_buffer_.write_queue_;
        outbuf.push_back(data);
    }

    void Stream::WantRecv()
    {
        io_buffer_.read_any_ = true;

        io_buffer_.read_bytes_ = 0;
        io_buffer_.bytes_needed_ = 0;
    }

    void Stream::WantRecv(std::size_t bytes_needed)
    {
        io_buffer_.read_any_ = false;

        io_buffer_.read_bytes_ = 0;
        io_buffer_.bytes_needed_ = bytes_needed;
    }

    void Stream::onSend()
    {
        auto& outbuf = io_buffer_.write_queue_;

        while(!outbuf.empty())
        {
            auto& msg = outbuf.front();

            auto buf = msg.data()+io_buffer_.write_queue_head_offset_;
            auto len = msg.size()-io_buffer_.write_queue_head_offset_;

            auto written = ::send(conn_fd_, buf, len, 0);

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
        vector<Byte> recv_data;

        io_buffer_.read_any_?
            recvRaw():
            recvAdv();

        if (io_buffer_.bytes_needed_ != 0)
        {
            /* incomplete data */
            return;
        }

        recv_data.assign(io_buffer_.read_buffer_.begin(), io_buffer_.read_buffer_.begin()+io_buffer_.read_bytes_);

        io_buffer_.read_bytes_ = 0;
        io_buffer_.bytes_needed_ = 0;

        // invoke callback
        if (recv_callback_ != nullptr)
        {
            recv_callback_(recv_data);
        }
    }

    void Stream::recvAdv()
    {
        auto& inbuf = io_buffer_.read_buffer_;

        assert(io_buffer_.read_bytes_+io_buffer_.bytes_needed_ <= inbuf.size());

        auto buf = inbuf.data() + io_buffer_.read_bytes_;
        auto len = io_buffer_.bytes_needed_;

        auto bytes_in = ::recv(conn_fd_, buf, len, 0);
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

    void Stream::recvRaw()
    {
        auto& inbuf = io_buffer_.read_buffer_;

        auto buf = inbuf.data();
        auto len = inbuf.size();

        auto bytes_in = ::recv(conn_fd_, buf, len, 0);
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

        io_buffer_.read_bytes_ = bytes_in;
    }
}