#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "epoll.h"
#include "connector.h"

using namespace loquat;

class ChatClient : public Connector
{
    public:
        ChatClient(std::string name) : name_(name), Connector() {};

        void OnRecv(std::vector<Byte>& data) override
        {
            std::cout << data.data() << std::endl;

            Epoll::GetInstance().Terminate();
        }

        void Say(const std::string& message)
        {
            std::stringstream sentance;
            sentance << name_ << ": " << message;

            auto msg = sentance.str();
            Enqueue(std::vector<Byte>(msg.data(), msg.data()+msg.size()));
        }
    private:
        std::string name_;
};

int main( int argc,      // Number of strings in array argv
          char *argv[],   // Array of command-line argument strings
          char *envp[] )  // Array of environment variable strings
{
    auto p_connector = std::make_shared<ChatClient>("Emma");

    Epoll::GetInstance().Join(p_connector->Sock(), p_connector);

    p_connector->Say("Hello World");

    p_connector->Connect("127.0.0.1", 300158);

    Epoll::GetInstance().Wait();

    Epoll::GetInstance().Leave(p_connector->Sock());

    return 0;
}