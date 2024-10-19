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
        void OnRecv(std::vector<loquat::Byte> &data) override
        {
            EXPECT_EQ(data, stringToVector("Good to see you too."));

            loquat::Epoll::GetInstance().Terminate();
        }
    };

    class TestConnection : public loquat::Connection
    {
    public:
        TestConnection(int listen_fd) : Connection(listen_fd) {}

        void OnRecv(std::vector<loquat::Byte> &data) override
        {
            EXPECT_EQ(data, stringToVector("Em, it's happy to see you."));

            Connection::Enqueue(stringToVector("Good to see you too."));
        }
    };

    class TestListener : public loquat::Listener
    {
    public:
        static const int kMaxConnections = 20;

        TestListener() : Listener(kMaxConnections) {};

        void OnAccept(int listen_sock) override
        {
            auto connection_ptr = std::make_shared<TestConnection>(listen_sock);
            loquat::Epoll::GetInstance().Join(connection_ptr->Sock(), connection_ptr);
        }
    };

    TEST(Stream_IPv4, send_receive)
    {
        auto p_listener = std::make_shared<TestListener>();
        loquat::Epoll::GetInstance().Join(p_listener->Sock(), p_listener);
        p_listener->Listen("127.0.0.1", 16138);

        auto p_connector = std::make_shared<TestConnector>();
        loquat::Epoll::GetInstance().Join(p_connector->Sock(), p_connector);
        p_connector->Bind("127.0.0.1", 27149);
        p_connector->Connect("127.0.0.1", 16138);

        std::future<void> fut = std::async(std::launch::async, []
                                           { loquat::Epoll::GetInstance().Wait(); });

        p_connector->Enqueue(stringToVector("Em, it's happy to see you."));

        fut.wait();

        loquat::Epoll::GetInstance().Leave(p_connector->Sock());
        loquat::Epoll::GetInstance().Leave(p_listener->Sock());
    }

    class TestShouter : public loquat::Connector
    {
    public:
        void OnRecv(std::vector<loquat::Byte> &data) override
        {
            Echoes.insert(Echoes.end(), data.begin(), data.end());

            if (data.size() == 4)
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
        TestEcho(int listen_fd) : Connection(listen_fd) {}

        void OnRecv(std::vector<loquat::Byte> &data) override
        {
            Connection::Enqueue(data);
        }
    };

    class TestEchoListener : public loquat::Listener
    {
    public:
        static const int kMaxConnections = 20;

        TestEchoListener() : Listener(kMaxConnections) {};

        void OnAccept(int listen_sock) override
        {
            auto connection_ptr = std::make_shared<TestEcho>(listen_sock);
            loquat::Epoll::GetInstance().Join(connection_ptr->Sock(), connection_ptr);
        }
    };

    TEST(Stream_IPv4, 10kRuns)
    {
        auto p_listener = std::make_shared<TestEchoListener>();
        loquat::Epoll::GetInstance().Join(p_listener->Sock(), p_listener);
        p_listener->Listen("127.0.0.1", 16138);

        auto p_connector = std::make_shared<TestShouter>();
        loquat::Epoll::GetInstance().Join(p_connector->Sock(), p_connector);
        p_connector->Bind("127.0.0.1", 27149);
        p_connector->Connect("127.0.0.1", 16138);

        std::future<void> fut = std::async(std::launch::async, []
                                           { loquat::Epoll::GetInstance().Wait(); });

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> length_dist(100, 1024);
        std::uniform_int_distribution<> value_dist(0, 255);

        for (int c = 0; c < 10 * 1000; c++)
        {
            int length = length_dist(gen);
            std::vector<loquat::Byte> data(length);

            for (int i = 0; i < length; ++i)
            {
                data[i] = static_cast<loquat::Byte>(value_dist(gen));
            }

            p_connector->Enqueue(data);
        }

        p_connector->Enqueue(stringToVector("EXIT"));

        fut.wait();

        EXPECT_EQ(p_connector->Shouts, p_connector->Echoes);

        loquat::Epoll::GetInstance().Leave(p_connector->Sock());
        loquat::Epoll::GetInstance().Leave(p_listener->Sock());
    }
}