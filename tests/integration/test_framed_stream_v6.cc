#include <future>
#include <random>

#include "listener.h"
#include "connector.h"
#include "epoll.h"

#include "gtest/gtest.h"
namespace
{
    std::vector<unsigned char> stringToVector(const std::string &str)
    {
        return std::vector<unsigned char>(str.begin(), str.end());
    }

    class TestConnector : public loquat::Connector
    {
    public:
        TestConnector() : Connector(Stream::Type::Framed, AF_INET6)
        {
            current_bytes_need_ = 4;
            SetBytesNeeded(current_bytes_need_);
        }

        void OnRecv(const std::vector<loquat::Byte> &data) override
        {
            EXPECT_EQ(data.size(), current_bytes_need_);

            current_bytes_need_ = 1024;
            SetBytesNeeded(current_bytes_need_);

            if (data.size() == 1024)
            {
            loquat::Epoll::GetInstance().Terminate();
        }
        }

    private:
        int current_bytes_need_;
    };

    class TestConnection : public loquat::Connection
    {
    public:
        TestConnection(int listen_fd) : Connection(Stream::Type::Framed, listen_fd)
        {
            current_bytes_need_ = 4;
            SetBytesNeeded(current_bytes_need_);
        }

        void OnRecv(const std::vector<loquat::Byte> &data) override
        {
            EXPECT_EQ(data.size(), current_bytes_need_);

            current_bytes_need_ = 1024;
            SetBytesNeeded(current_bytes_need_);

            Connection::Enqueue(data);
        }

    private:
        int current_bytes_need_;
    };

    class TestListener : public loquat::Listener
    {
    public:
        static const int kMaxConnections = 20;

        TestListener() : Listener(AF_INET6, kMaxConnections) {};
        ~TestListener()
        {
            loquat::Epoll::GetInstance().Leave(connection_ptr->Sock());
        }

        void OnAccept(int listen_sock) override
        {
            connection_ptr = std::make_shared<TestConnection>(listen_sock);
            loquat::Epoll::GetInstance().Join(connection_ptr->Sock(), connection_ptr);
        }

    private:
        std::shared_ptr<TestConnection> connection_ptr;
    };

    TEST(FramedStream_IPv6, send_receive)
    {
        auto p_listener = std::make_shared<TestListener>();
        loquat::Epoll::GetInstance().Join(p_listener->Sock(), p_listener);
        p_listener->Listen("::1", 170504);

        auto p_connector = std::make_shared<TestConnector>();
        loquat::Epoll::GetInstance().Join(p_connector->Sock(), p_connector);
        p_connector->Bind("::1", 230510);
        p_connector->Connect("::1", 170504);

        std::future<void> fut = std::async(std::launch::async, []
                                           { loquat::Epoll::GetInstance().Wait(); });

        p_connector->Enqueue(std::vector<loquat::Byte>(4));

        p_connector->Enqueue(std::vector<loquat::Byte>(1024));

        fut.get();

        loquat::Epoll::GetInstance().Leave(p_connector->Sock());
        loquat::Epoll::GetInstance().Leave(p_listener->Sock());
    }

    class TestShouter : public loquat::Connector
    {
    public:
        TestShouter() : Connector(Stream::Type::Framed, AF_INET6)
        {
            SetBytesNeeded(4);
        }

        void OnRecv(const std::vector<loquat::Byte> &data) override
        {
            Echoes.insert(Echoes.end(), data.begin(), data.end());

            EXPECT_EQ(data.size(), 4);

            std::string flag(data.begin(), data.end());
                if (flag == "EXIT")
                {
                    loquat::Epoll::GetInstance().Terminate();
            }
        }

        void Enqueue(const std::vector<loquat::Byte> &data)
        {
            Connector::Enqueue(data);

            Shouts.insert(Shouts.end(), data.begin(), data.end());
        }

        std::vector<loquat::Byte> Shouts;
        std::vector<loquat::Byte> Echoes;
    };

    class TestEcho : public loquat::Connection
    {
    public:
        TestEcho(int listen_fd) : Connection(Stream::Type::Framed, listen_fd)
        {
            SetBytesNeeded(4);
        }

        void OnRecv(const std::vector<loquat::Byte> &data) override
        {
            EXPECT_EQ(data.size(), 4);
            Connection::Enqueue(data);
        }
    };

    class TestEchoListener : public loquat::Listener
    {
    public:
        static const int kMaxConnections = 20;

        TestEchoListener() : Listener(AF_INET6, kMaxConnections) {};
        ~TestEchoListener()
        {
            loquat::Epoll::GetInstance().Leave(connection_ptr->Sock());
        }

        void OnAccept(int listen_sock) override
        {
            connection_ptr = std::make_shared<TestEcho>(listen_sock);
            loquat::Epoll::GetInstance().Join(connection_ptr->Sock(), connection_ptr);
        }

    private:
        std::shared_ptr<TestEcho> connection_ptr;
    };

    TEST(FramedStream_IPv6, 10kRuns)
    {
        auto p_listener = std::make_shared<TestEchoListener>();
        loquat::Epoll::GetInstance().Join(p_listener->Sock(), p_listener);
        p_listener->Listen("::1", 170504);

        auto p_connector = std::make_shared<TestShouter>();
        loquat::Epoll::GetInstance().Join(p_connector->Sock(), p_connector);
        p_connector->Bind("::1", 230510);
        p_connector->Connect("::1", 170504);

        std::future<void> fut = std::async(std::launch::async, []
                                           { loquat::Epoll::GetInstance().Wait(); });

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> value_dist(0, 255);

        for (int c = 0; c < 10 * 1000; c++)
        {
            int length = 20;
            std::vector<loquat::Byte> data(length);

            for (int i = 0; i < length; ++i)
            {
                data[i] = static_cast<loquat::Byte>(value_dist(gen));
            }

            p_connector->Enqueue(data);
        }

        p_connector->Enqueue(stringToVector("EXIT"));

        fut.get();

        EXPECT_EQ(p_connector->Shouts.size(), p_connector->Echoes.size());
        EXPECT_EQ(p_connector->Shouts, p_connector->Echoes);

        loquat::Epoll::GetInstance().Leave(p_connector->Sock());
        loquat::Epoll::GetInstance().Leave(p_listener->Sock());
    }
}
