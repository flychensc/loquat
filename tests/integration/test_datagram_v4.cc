#include <future>
#include <random>

#include "peer.h"
#include "epoll.h"

#include "gtest/gtest.h"
namespace
{
    std::vector<unsigned char> stringToVector(const std::string &str)
    {
        return std::vector<unsigned char>(str.begin(), str.end());
    }

    class TestPeerS : public loquat::Peer
    {
    public:
        void OnRecv(struct sockaddr &fromaddr, socklen_t addrlen, std::vector<loquat::Byte> &data) override
        {
            EXPECT_EQ(data, stringToVector("Em, it's happy to see you."));

            Datagram::Enqueue(fromaddr, addrlen, stringToVector("Good to see you too."));
        }
    };

    class TestPeerC : public loquat::Peer
    {
    public:
        void OnRecv(struct sockaddr &fromaddr, socklen_t addrlen, std::vector<loquat::Byte> &data) override
        {
            EXPECT_EQ(data, stringToVector("Good to see you too."));

            loquat::Epoll::GetInstance().Terminate();
        }
    };

    TEST(Datagram_IPv4, send_receive)
    {
        auto p_peer_s = std::make_shared<TestPeerS>();
        loquat::Epoll::GetInstance().Join(p_peer_s->Sock(), p_peer_s);
        p_peer_s->Bind("127.0.0.1", 16138);

        auto p_peer_c = std::make_shared<TestPeerC>();
        loquat::Epoll::GetInstance().Join(p_peer_c->Sock(), p_peer_c);
        p_peer_c->Bind("127.0.0.1", 27149);

        std::future<void> fut = std::async(std::launch::async, []
                                           { loquat::Epoll::GetInstance().Wait(); });

        p_peer_c->Enqueue("127.0.0.1", 16138, stringToVector("Em, it's happy to see you."));

        fut.wait();

        loquat::Epoll::GetInstance().Leave(p_peer_c->Sock());
        loquat::Epoll::GetInstance().Leave(p_peer_s->Sock());
    }

    class TestEcho : public loquat::Peer
    {
    public:
        void OnRecv(struct sockaddr &fromaddr, socklen_t addrlen, std::vector<loquat::Byte> &data) override
        {
            Datagram::Enqueue(fromaddr, addrlen, data);
        }
    };

    class TestShouter : public loquat::Peer
    {
    public:
        void OnRecv(struct sockaddr &fromaddr, socklen_t addrlen, std::vector<loquat::Byte> &data) override
        {
            Echoes.insert(Echoes.end(), data.begin(), data.end());

            if (data.size() == 4)
            {
                loquat::Epoll::GetInstance().Terminate();
            }
        }

        void Enqueue(const std::string& to_ip, int port, const std::vector<loquat::Byte> &data)
        {
            Peer::Enqueue(to_ip, port, data);

            Shouts.insert(Shouts.end(), data.begin(), data.end());
        }

        std::vector<loquat::Byte> Shouts;
        std::vector<loquat::Byte> Echoes;
    };

    TEST(Datagram_IPv4, 10kRuns)
    {
        auto p_peer_s = std::make_shared<TestEcho>();
        loquat::Epoll::GetInstance().Join(p_peer_s->Sock(), p_peer_s);
        p_peer_s->Bind("127.0.0.1", 16138);

        auto p_peer_c = std::make_shared<TestShouter>();
        loquat::Epoll::GetInstance().Join(p_peer_c->Sock(), p_peer_c);
        p_peer_c->Bind("127.0.0.1", 27149);

        std::future<void> fut = std::async(std::launch::async, []
                                           { loquat::Epoll::GetInstance().Wait(); });

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> length_dist(100, 1024);
        std::uniform_int_distribution<> value_dist(0, 255);

        for (int i = 0; i < 10 * 1000; i++)
        {
            int length = length_dist(gen);
            std::vector<loquat::Byte> data(length);

            for (int i = 0; i < length; ++i)
            {
                data[i] = static_cast<loquat::Byte>(value_dist(gen));
            }

            p_peer_c->Enqueue("127.0.0.1", 16138, data);
        }

        p_peer_c->Enqueue("127.0.0.1", 16138, stringToVector("EXIT"));

        fut.wait();

        EXPECT_EQ(p_peer_c->Shouts, p_peer_c->Echoes);

        loquat::Epoll::GetInstance().Leave(p_peer_c->Sock());
        loquat::Epoll::GetInstance().Leave(p_peer_s->Sock());
    }
}
