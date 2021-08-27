#ifndef TCPSOCKET_H
#define TCPSOCKET_H

#include <future>
#include <memory>
#include <string_view>

#include <stdint.h>

#include "../nativeSocket/nativeSocket.h"
#include "../socketBase/socketBase.h"


namespace kleins {
    class tcpSocket : public socketBase
    {
    private:
        nativeSocket skt;

        virtual bool tick() final;

    public:
        tcpSocket(std::string_view listenAddress, uint16_t listenPort);
        tcpSocket(const tcpSocket&) = delete;
        ~tcpSocket();

        virtual std::future<bool> init() final;
    };
    
};

#endif