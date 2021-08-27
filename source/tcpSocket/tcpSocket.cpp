#include "tcpSocket.h"

#include "../tcpConnection/tcpConnection.h"

kleins::tcpSocket::tcpSocket(std::string_view listenAddress, uint16_t listenPort) :
    skt(AI_FAMILY::INET, AI_SOCKTYPE::STREAM, AI_PROTOCOL::TCP)
{

    auto hint = addrInfo::builder().family(AI_FAMILY::INET)
                                   .socktype(AI_SOCKTYPE::STREAM)
                                   .protocol(AI_PROTOCOL::TCP)
                                   .get();
    std::list<addrInfo> addresses;
    auto err = hint->getAddrInfo(listenAddress, std::to_string(listenPort), addresses);
    if (err != AI_ERROR::NONE) {
        perror("getAddrInfo");
        exit(EXIT_FAILURE);
    }

    bool addrFound = false;
    for (auto &address : addresses) {
        SKT_ERROR bind_err = skt.bind(address);
        if (bind_err != SKT_ERROR::NONE)
            continue;
        addrFound = true;
        break;
    }

    if (!addrFound) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
}

kleins::tcpSocket::~tcpSocket()
{
}

bool kleins::tcpSocket::tick()
{
    auto [newConnection, err] = skt.accept();
    tcpConnection* conn = new tcpConnection(newConnection);
    newConnectionCallback(conn); 

    return newConnection->valid();
}

std::future<bool> kleins::tcpSocket::init()
{
    auto init_async = [this](){

        int opt = 1;
        SKT_ERROR err = skt.setOpt(OPT_LEVEL::SOCKET,
                                   SKT_OPTION::REUSEADDR,
                                   &opt, sizeof(opt));
        if (err != SKT_ERROR::NONE) {
            std::cerr << "Error setting socket opts" << std::endl;
            return false;
        }

        err = skt.listen(3);
        if (err != SKT_ERROR::NONE) {
            std::cerr << "Error listening on socket" << std::endl;
            return false;
        }

        return true;
    };

    return std::async(std::launch::async, init_async);
}