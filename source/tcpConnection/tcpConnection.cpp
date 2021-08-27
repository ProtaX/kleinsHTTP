#include "tcpConnection.h"

#include "../packet/packet.h"

#include <unistd.h>

kleins::tcpConnection::tcpConnection(nativeSocket *s)
{
    skt = s;
}

kleins::tcpConnection::~tcpConnection()
{
    close_socket();
    join();
}

bool kleins::tcpConnection::getAlive()
{
    int error_opt = 0;
    size_t error_opt_len = sizeof(error_opt);
    SKT_ERROR err = skt->getOpt(OPT_LEVEL::SOCKET, SKT_OPTION::ERROR_, &error_opt, &error_opt_len);

    return (!error_opt && err == SKT_ERROR::NONE);
}

void kleins::tcpConnection::tick()
{
    packet* packetBuffer = new packet;

    packetBuffer->data.resize(4096);
    auto res = skt->recv(4096, &packetBuffer->data[0], MSG_FLAGS::DONTWAIT);
    
    if(res.second != SKT_ERROR::NONE)
    {
        delete packetBuffer;
        usleep(20000);
        return;
    }

    packetBuffer->size = res.first;
    packetBuffer->data.resize(packetBuffer->size);

    this->onRecieveCallback(std::unique_ptr<packet>(packetBuffer));
}

void kleins::tcpConnection::sendData(const char* data, int datalength)
{
    skt->send(data, datalength, MSG_FLAGS::NONE);
}

void kleins::tcpConnection::close_socket()
{
    skt->close();
}