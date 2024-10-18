#include "connector.h"

#include "gtest/gtest.h"
namespace
{
    class TestConnector : public loquat::Connector
    {
    public:
        void OnRecv(std::vector<loquat::Byte> &data) {}
    };

    TEST(Connector, destructor)
    {
        int socket;

        {
            TestConnector connector;
            socket = connector.Sock();
        }

        int error;
        socklen_t len = sizeof(error);
        int rv = getsockopt(socket, SOL_SOCKET, SO_ERROR, &error, &len);
        EXPECT_EQ(rv, -1);
    }
}
