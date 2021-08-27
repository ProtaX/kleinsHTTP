#ifndef CONNECTION_H
#define CONNECTION_H

#include "../connectionBase/connectionBase.h"
#include "../nativeSocket/nativeSocket.h"

namespace kleins {
    class tcpConnection : public connectionBase
    {
    private:
        nativeSocket *skt;

    public:
        tcpConnection(nativeSocket *s);
        ~tcpConnection();

        virtual bool getAlive();
        virtual void tick();
        virtual void sendData(const char* data, int datalength);
        virtual void close_socket();
    };
};

#endif