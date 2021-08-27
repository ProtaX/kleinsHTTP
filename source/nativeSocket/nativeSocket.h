#ifndef _NATIVESOCKET_H
#define _NATIVESOCKET_H

#include <stdint.h>
#include <stdlib.h>

#include <list>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#pragma comment(lib, "Ws2_32.lib")

namespace kleins {
  enum class AI_PROTOCOL {
    TCP = IPPROTO_TCP,
    UDP = IPPROTO_UDP,
  };

  enum class SKT_ERROR {
    NONE,
    AGAIN = WSAEINPROGRESS,
    WOULDBLOCK = WSAEWOULDBLOCK,
    BADF = WSAEBADF,
    CONNREFUSED = WSAECONNREFUSED,
    FAULT = WSAEFAULT,
    INTR = WSAEINTR,
    INVAL = WSAEINVAL,
    NOBUFS = WSAENOBUFS,
    NOMEM = WSA_NOT_ENOUGH_MEMORY,
    NOTCONN = WSAENOTCONN,
    NOTSOCK = WSAENOTSOCK,
    ACCESS = WSAEACCES,
    AFNOSUPPORT = WSAEAFNOSUPPORT,
    MFILE = WSAEMFILE,
    PROTONOSUPPORT = WSAEPFNOSUPPORT,
  };

  enum MSG_FLAGS {
    NONE = 0,
    OOB = MSG_OOB,
    PEEK = MSG_PEEK,
    TRUNC = MSG_TRUNC,
    WAITALL = MSG_WAITALL,
    CTRUNC = MSG_CTRUNC,
    ERRQUEUE = MSG_ERRQUEUE,
    DONTWAIT,  // keep last
  };

  enum class SD_HOW {
    RECEIVE = SD_RECEIVE,
    SEND = SD_SEND,
    BOTH = SD_BOTH,
  };

}
#elif defined(__linux__)
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>

namespace kleins {
  enum class AI_PROTOCOL {
    TCP = 0,
    UDP = 0,
  };

  enum class SKT_ERROR {
    NONE = 0,
    AGAIN = EAGAIN,
    WOULDBLOCK = EWOULDBLOCK,
    BADF = EBADF,
    CONNREFUSED = ECONNREFUSED,
    FAULT = EFAULT,
    INTR = EINTR,
    INVAL = EINVAL,
    NOBUFS = ENOBUFS,
    NOMEM = ENOMEM,
    NOTCONN = ENOTCONN,
    NOTSOCK = ENOTSOCK,
    ACCES = EACCES,
    AFNOSUPPORT = EAFNOSUPPORT,
    MFILE = EMFILE,
    PROTONOSUPPORT = EPROTONOSUPPORT,
  };

  enum MSG_FLAGS {
    NONE = 0,
    ERRQUEUE = MSG_ERRQUEUE,
    OOB = MSG_OOB,
    PEEK = MSG_PEEK,
    TRUNC = MSG_TRUNC,
    WAITALL = MSG_WAITALL,
    CTRUNC = MSG_CTRUNC,
    DONTWAIT = MSG_DONTWAIT,
  };

  enum SD_HOW {
    RECEIVE = SHUT_RD,
    SEND = SHUT_WR,
    BOTH = SHUT_RDWR
  };
}
#else
    #error "unknown socket platform"
#endif

namespace kleins {

  enum class AI_FAMILY {
    UNSPEC = AF_UNSPEC,
    INET = AF_INET,
    INET6 = AF_INET6,
  };

  enum class AI_SOCKTYPE {
    STREAM = SOCK_STREAM,
    DGRAM = SOCK_DGRAM,
    RAW = SOCK_RAW,
    RDM = SOCK_RDM,
    SEQPACKET = SOCK_SEQPACKET,
  };

  enum AI_FLAGS {
    PASSIVE = AI_PASSIVE,
    ADDRCONFIG = AI_ADDRCONFIG,
    NUMERICHOST = AI_NUMERICHOST,
    NUMERICSERV = AI_NUMERICSERV,
    CANONNAME = AI_CANONNAME,
    ALL = AI_ALL,
    V4MAPPED = AI_V4MAPPED,
  };

  enum class OPT_LEVEL {
    SOCKET = SOL_SOCKET,
  };

  enum class SKT_OPTION {
    ERROR_ = SO_ERROR,
    BROADCAST = SO_BROADCAST,
    ACCEPTCONN = SO_ACCEPTCONN,
    DEBUG = SO_DEBUG,
    DONTROUTE = SO_DONTROUTE,
    KEEPALIVE = SO_KEEPALIVE,
    LINGER = SO_LINGER,
    OOBINLINE = SO_OOBINLINE,
    RCVBUF = SO_RCVBUF,
    RCVLOWAT = SO_RCVLOWAT,
    REUSEADDR = SO_REUSEADDR,
    RCVTIMEO = SO_RCVTIMEO,
    SNDBUF = SO_SNDBUF,
    SNDLOWAT = SO_SNDLOWAT,
    SNDTIMEO = SO_SNDTIMEO,
    TYPE = SO_TYPE,
  };

  enum class AI_ERROR {
    NONE = 0,
    AGAIN = EAI_AGAIN,
    BADFLAGS = EAI_BADFLAGS,
    FAIL = EAI_FAIL,
    FAMILY = EAI_FAMILY,
    MEMORY = EAI_MEMORY,
    NODATA = EAI_NODATA,
    NONAME = EAI_NONAME,
    SERVICE = EAI_SERVICE,
    SOCKTYPE = EAI_SOCKTYPE,
  };

  class INativeWrapper {
  public:
    virtual std::pair<const void*, size_t> getNative() const = 0;
  };

  class addrInfo : INativeWrapper {
  private:
    friend class builder;
    struct addrinfo info;

    addrInfo();
    addrInfo(const struct addrinfo *addr);

  public:
    class builder {
    private:
      friend class addrInfo;
      std::unique_ptr<addrInfo> obj;

    public:
      builder();
      ~builder() = default;

      void create();
      std::unique_ptr<addrInfo>&& get();

      builder& family(AI_FAMILY family);
      builder& socktype(AI_SOCKTYPE socktype);
      builder& protocol(AI_PROTOCOL protocol);
      builder& flags(AI_FLAGS flags);
    };

    template<
      template<typename, typename>
      typename TList
    >
    AI_ERROR getAddrInfo(std::string_view name, std::string_view service, TList<addrInfo, std::allocator<addrInfo>>& c) const {
      struct addrinfo *res = nullptr;
      AI_ERROR err = (AI_ERROR)::getaddrinfo(name.data(), service.data(), &info, &res);
      if (err != AI_ERROR::NONE)
          return err;
      
      for (; res != nullptr; res = res->ai_next)
          c.insert(c.begin(), addrInfo{res});
      
      ::freeaddrinfo(res);
      return AI_ERROR::NONE;
    }

    std::pair<const void*, size_t> getNative() const final;
  };

  class nativeSocket : INativeWrapper {
  private:
    struct Impl;
    std::unique_ptr<Impl> impl;

    nativeSocket(std::unique_ptr<Impl>&& i) noexcept;

  public:
    nativeSocket(AI_FAMILY family, AI_SOCKTYPE type, AI_PROTOCOL proto) noexcept;

    nativeSocket(const nativeSocket&) = delete;

    ~nativeSocket();

    bool valid() const;

    SKT_ERROR bind(const addrInfo &addr);

    SKT_ERROR listen(size_t queueLen = SOMAXCONN);

    SKT_ERROR setOpt(OPT_LEVEL level, SKT_OPTION optname, const void *optval, size_t optlen);

    SKT_ERROR getOpt(OPT_LEVEL level, SKT_OPTION optname, void *optval, size_t *optlen);

    SKT_ERROR connect(const addrInfo &addr);

    std::pair<nativeSocket*, SKT_ERROR> accept();

    std::pair<size_t, SKT_ERROR> recv(size_t len, char *buf, MSG_FLAGS flags);

    std::pair<size_t, SKT_ERROR> send(const char *buf, size_t len, MSG_FLAGS flags);

    std::pair<const void*, size_t> getNative() const final;

    SKT_ERROR close();

    SKT_ERROR shutdown(SD_HOW how);
  };
}

#endif // _NATIVESOCKET_H