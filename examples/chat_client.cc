#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

#include "epoll.h"
#include "connector.h"

using namespace loquat;

class ChatClient : public Connector
{
public:
    ChatClient(std::string name) : name_(name), Connector() {}

    void OnRecv(const std::vector<Byte> &data) override
    {
        std::string str(data.begin(), data.end());
        std::cout << str << std::endl;
    }

    void Say(const std::string &message)
    {
        std::stringstream sentance;
        sentance << name_ << ": " << message;

        auto msg = sentance.str();
        Enqueue(std::vector<Byte>(msg.data(), msg.data() + msg.size()));
    }

private:
    std::string name_;
};

class Console : public ReadWritable
{
public:
    Console(std::shared_ptr<ChatClient> client_ptr) : client_(client_ptr) {}
    void OnWrite(int sock_fd) override { Epoll::GetInstance()->DataOutClear(STDIN_FILENO); }
    void OnRead(int sock_fd) override
    {
        std::string msg;
        std::getline(std::cin, msg);

        // detect `quit`
        // Epoll::GetInstance()->Terminate();

        if (!client_.expired())
        {
            auto client_ptr = client_.lock();
            client_ptr->Say(msg);
        }
    }

private:
    std::weak_ptr<ChatClient> client_;
};

int main(int argc,     // Number of strings in array argv
         char *argv[], // Array of command-line argument strings
         char *envp[]) // Array of environment variable strings
{
    const char *name = "Emma"; // default
    if (argc > 1)
    {
        name = argv[1];
    }
    auto p_client = std::make_shared<ChatClient>(name);
    Epoll::GetInstance()->Join(p_client->Sock(), p_client);

    auto p_console = std::make_shared<Console>(p_client);
    Epoll::GetInstance()->Join(STDIN_FILENO, p_console);

    p_client->Connect("127.0.0.1", 300158);

    Epoll::GetInstance()->Wait();

    Epoll::GetInstance()->Leave(p_client->Sock());

    return 0;
}