#include <future>

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
        TestPeerS() : loquat::Peer(AF_UNIX) {}

        void OnRecv(struct sockaddr &fromaddr, socklen_t addrlen, std::vector<loquat::Byte> &data) override
        {
            EXPECT_EQ(data, stringToVector("Em, it's happy to see you."));

            Datagram::Enqueue(fromaddr, addrlen, stringToVector("Good to see you too."));
        }
    };

    class TestPeerC : public loquat::Peer
    {
    public:
        TestPeerC() : loquat::Peer(AF_UNIX) {}

        void OnRecv(struct sockaddr &fromaddr, socklen_t addrlen, std::vector<loquat::Byte> &data) override
        {
            EXPECT_EQ(data, stringToVector("Good to see you too."));

            loquat::Epoll::GetInstance().Terminate();
        }
    };

    TEST(Peer, send_receive)
    {
        auto p_peer_s = std::make_shared<TestPeerS>();
        loquat::Epoll::GetInstance().Join(p_peer_s->Sock(), p_peer_s);
        p_peer_s->Bind("/tmp/peer_s");

        auto p_peer_c = std::make_shared<TestPeerC>();
        loquat::Epoll::GetInstance().Join(p_peer_c->Sock(), p_peer_c);
        p_peer_c->Bind("/tmp/peer_c");

        std::future<void> fut = std::async(std::launch::async, []
                                           { loquat::Epoll::GetInstance().Wait(); });

        p_peer_c->Enqueue("/tmp/peer_s", stringToVector("Em, it's happy to see you."));

        loquat::Epoll::GetInstance().Leave(p_peer_c->Sock());
        loquat::Epoll::GetInstance().Leave(p_peer_s->Sock());

        fut.wait();
    }
}
