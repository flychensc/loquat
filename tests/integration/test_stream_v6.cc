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
        TestConnector() : Connector(AF_INET6) {}

        void OnRecv(const std::vector<loquat::Byte> &data) override
        {
            EXPECT_EQ(data, stringToVector("Good to see you too."));

            loquat::Epoll::GetInstance().Terminate();
        }
    };

    class TestConnection : public loquat::Connection
    {
    public:
        TestConnection(int listen_fd) : Connection(listen_fd) {}

        void OnRecv(const std::vector<loquat::Byte> &data) override
        {
            EXPECT_EQ(data, stringToVector("Em, it's happy to see you."));

            Connection::Enqueue(stringToVector("Good to see you too."));
        }
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

    TEST(Stream_IPv6, send_receive)
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

        p_connector->Enqueue(stringToVector("Em, it's happy to see you."));

        fut.get();

        loquat::Epoll::GetInstance().Leave(p_connector->Sock());
        loquat::Epoll::GetInstance().Leave(p_listener->Sock());
    }

    class TestShouter : public loquat::Connector
    {
    public:
        TestShouter() : Connector(AF_INET6) {}

        void OnRecv(const std::vector<loquat::Byte> &data) override
        {
            Echoes.insert(Echoes.end(), data.begin(), data.end());

            if (Echoes.size() >= 4)
            {
                std::string flag(reinterpret_cast<char *>(Echoes.data()) + (Echoes.size() - 4), 4);
                if (flag == "EXIT")
                {
                    loquat::Epoll::GetInstance().Terminate();
                }
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

        void OnRecv(const std::vector<loquat::Byte> &data) override
        {
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

    TEST(Stream_IPv6, 10kRuns)
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
        std::uniform_int_distribution<> length_dist(100, 4 * 1024);
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

        fut.get();

        EXPECT_EQ(p_connector->Shouts.size(), p_connector->Echoes.size());
        EXPECT_EQ(p_connector->Shouts, p_connector->Echoes);

        loquat::Epoll::GetInstance().Leave(p_connector->Sock());
        loquat::Epoll::GetInstance().Leave(p_listener->Sock());
    }
}
