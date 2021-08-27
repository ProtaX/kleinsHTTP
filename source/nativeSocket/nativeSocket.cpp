/// Common headers
#include "nativeSocket.h"

#include <cstring>
#include <utility>

/// Platform-specific headers
#ifdef _WIN32

#elif defined(__linux__)
    #include <errno.h>
    #include <sys/types.h>
    #include <netdb.h>
    #include <unistd.h>
#endif

/// Platform-specific data
namespace kleins {
    struct nativeSocket::Impl {
    #ifdef _WIN32
        SOCKET skt;
        static bool init();
        static WSADATA wsaData;
        static bool initialized;
    #elif defined(__linux__)
        int skt;
    #endif
    };
}

/// Common behaviour
namespace kleins
{
    addrInfo::addrInfo() {
      info.ai_next = nullptr;
    }

    addrInfo::addrInfo(const struct addrinfo *addr) {
      info = *addr;
    }

    addrInfo::builder::builder() {
      create();
    }

    void addrInfo::builder::create() {
      auto p = new addrInfo();
      obj = std::unique_ptr<addrInfo>(p);
    }

    std::unique_ptr<addrInfo>&& addrInfo::builder::get() {
      return std::move(obj);
    }

    std::pair<const void*, size_t> addrInfo::getNative() const {
        return std::pair(&info, sizeof(info));
    }

    addrInfo::builder& addrInfo::builder::family(AI_FAMILY family) {
        obj->info.ai_family = (int)family;
        return *this;
    }

    addrInfo::builder& addrInfo::builder::protocol(AI_PROTOCOL protocol) {
        obj->info.ai_protocol = (int)protocol;
        return *this;
    }

    addrInfo::builder& addrInfo::builder::socktype(AI_SOCKTYPE socktype) {
        obj->info.ai_socktype = (int)socktype;
        return *this;
    }

    addrInfo::builder& addrInfo::builder::flags(AI_FLAGS flags) {
        obj->info.ai_flags = (int)flags;
        return *this;
    }
}


/// Platform-specific behaviour
#ifdef _WIN32

namespace kleins {

  bool nativeSocket::Impl::initialized = false;

  nativeSocket::nativeSocket(AI_FAMILY family, AI_SOCKTYPE type, AI_PROTOCOL proto) {
    impl = std::make_unique<Impl>();

    impl->init();
    SOCKET skt = ::socket((int)family, (int)type, (int)proto);
    if (impl->skt == INVALID_SOCKET) {
      perror("socket");
      exit(EXIT_FAILURE);
    }

    impl->skt = skt;
  }

  nativeSocket::nativeSocket(std::unique_ptr<Impl>&& i) noexcept {
    impl = std::make_unique<Impl>();
    impl->init();
    impl->skt = i->skt;
    i->skt = INVALID_SOCKET;
  }

  nativeSocket::~nativeSocket() {
    impl->~Impl();
  }

  bool nativeSocket::valid() const {
    return impl->skt != INVALID_SOCKET;
  }

  SKT_ERROR nativeSocket::bind(const addrInfo &addr) {
    auto [address, sz] = addr.getNative();
    SKT_ERROR err = (SKT_ERROR)::bind(impl->skt, ((const addrinfo*)address)->ai_addr, (int)sz);
    if (err != SKT_ERROR::NONE)
        return err;
    return SKT_ERROR::NONE;
  }

  SKT_ERROR nativeSocket::listen(size_t queueLen = SOMAXCONN) {
    return (SKT_ERROR)::listen(impl->skt, (int)queueLen);
  }

  SKT_ERROR nativeSocket::setOpt(OPT_LEVEL level, SKT_OPTION optname, const void *optval, size_t optlen) {
    return (SKT_ERROR)::setsockopt(impl->skt, (int)level, (int)optname, (const char *)optval, (int)optlen);
  }

  SKT_ERROR nativeSocket::getOpt(OPT_LEVEL level, SKT_OPTION optname, void *optval, size_t *optlen) {
    return (SKT_ERROR)::getsockopt(impl->skt, (int)level, (int)optname, (char *)optval, (int*)optlen);
  }

  SKT_ERROR nativeSocket::connect(const addrInfo &addr) {
   auto [address, sz] = addr.getNative();
    SKT_ERROR err = (SKT_ERROR)::connect(impl->skt, ((const addrinfo*)address)->ai_addr, (int)sz);
    if (err != SKT_ERROR::NONE)
        return err;
    return SKT_ERROR::NONE;
  }

  std::pair<nativeSocket*, SKT_ERROR> nativeSocket::accept() {
    SOCKET s = ::accept(impl->skt, (sockaddr*)nullptr, (int*)nullptr);
    if (s == INVALID_SOCKET)
        return std::pair<nativeSocket*, SKT_ERROR>(nullptr, (SKT_ERROR)WSAGetLastError());
    auto i = std::make_unique<Impl>();
    i->skt = s;
    return std::pair<nativeSocket*, SKT_ERROR>(new nativeSocket(std::move(i)), SKT_ERROR::NONE);
  }

  std::pair<size_t, SKT_ERROR> nativeSocket::recv(size_t len, char *buf, MSG_FLAGS flags) {
    int res = ::recv(impl->skt, (char*)buf, (int)len, (int)flags);
    if (res < 0)
        return std::pair(0, (SKT_ERROR)WSAGetLastError());
    return std::pair(res, SKT_ERROR::NONE);
  }

  std::pair<size_t, SKT_ERROR> nativeSocket::send(const char *buf, size_t len, MSG_FLAGS flags) {
    int res = ::send(impl->skt, (const char *)buf, (int)len, (int)flags);
    if (res < 0)
        return std::pair(0, (SKT_ERROR)WSAGetLastError());
    return std::pair(res, SKT_ERROR::NONE);
  }

  std::pair<const void*, size_t> nativeSocket::getNative() const {
    return std::pair(&impl->skt, sizeof(SOCKET));
  }

  SKT_ERROR nativeSocket::close() {
    if (::closesocket(impl->skt) != 0)
        return (SKT_ERROR)WSAGetLastError();
    return SKT_ERROR::NONE;
  }

  SKT_ERROR nativeSocket::shutdown(SD_HOW how) {
    if (::shutdown(impl->skt, (int)how) != 0)
        return (SKT_ERROR)WSAGetLastError();
    return SKT_ERROR::NONE;
  }

  bool nativeSocket::Impl::init() {
    if (initialized)
        return true;

    int res = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (res != 0) {
        perror("Unable to initialize WinSock");
        return false;
    }
  }
}

#elif defined(__linux__)

namespace kleins {

  nativeSocket::nativeSocket(AI_FAMILY family, AI_SOCKTYPE type, AI_PROTOCOL proto) noexcept {
    impl = std::make_unique<Impl>();
    
    int skt = socket((int)family, (int)type, (int)proto);
    if (skt == -1) {
      perror("socket");
      exit(EXIT_FAILURE);
    }

    impl->skt = skt;
  }

  nativeSocket::nativeSocket(std::unique_ptr<Impl>&& i) noexcept {
    impl = std::make_unique<Impl>();
    impl->skt = i->skt;
    i->skt = -1;
  }

  nativeSocket::~nativeSocket() {
    impl->~Impl();
  }

  bool nativeSocket::valid() const {
    return impl->skt != -1;
  }

  SKT_ERROR nativeSocket::bind(const addrInfo &addr) {
    auto [address, sz] = addr.getNative();
    SKT_ERROR err = (SKT_ERROR)::bind(impl->skt, ((const addrinfo*)address)->ai_addr, sz);
    if (err != SKT_ERROR::NONE)
      return err;
    return SKT_ERROR::NONE;
  }

  SKT_ERROR nativeSocket::listen(size_t queueLen) {
    return (SKT_ERROR)::listen(impl->skt, (int)queueLen);
  }

  SKT_ERROR nativeSocket::setOpt(OPT_LEVEL level, SKT_OPTION optname, const void *optval, size_t optlen) {
    return (SKT_ERROR)::setsockopt(impl->skt, (int)level, (int)optname, optval, (socklen_t)optlen);
  }

  SKT_ERROR nativeSocket::getOpt(OPT_LEVEL level, SKT_OPTION optname, void *optval, size_t *optlen) {
    return (SKT_ERROR)::getsockopt(impl->skt, (int)level, (int)optname, optval, (socklen_t*)optlen);
  }

  SKT_ERROR nativeSocket::connect(const addrInfo &addr) {
    auto [address, sz] = addr.getNative();
    SKT_ERROR err = (SKT_ERROR)::connect(impl->skt, ((const addrinfo*)address)->ai_addr, sz);
    if (err != SKT_ERROR::NONE)
        return err;
    return SKT_ERROR::NONE;
  }

  std::pair<nativeSocket*, SKT_ERROR> nativeSocket::accept() {
    int s = ::accept(impl->skt, nullptr, nullptr);
    if (s < 0)
      return std::pair<nativeSocket*, SKT_ERROR>(nullptr, (SKT_ERROR)errno);
    auto i = std::make_unique<Impl>();
    i->skt = s;
    return std::pair<nativeSocket*, SKT_ERROR>(new nativeSocket(std::move(i)), SKT_ERROR::NONE);
  }

  std::pair<size_t, SKT_ERROR> nativeSocket::recv(size_t len, char *buf, MSG_FLAGS flags) {
    ssize_t res = ::recv(impl->skt, (void*)buf, len, (int)flags);
    if (res < 0)
      return std::pair(0, (SKT_ERROR)errno);
    return std::pair(res, SKT_ERROR::NONE);
  }

  std::pair<size_t, SKT_ERROR> nativeSocket::send(const char *buf, size_t len, MSG_FLAGS flags) {
    ssize_t res = ::send(impl->skt, (const void*)buf, len, (int)flags);
    if (res < 0)
      return std::pair(0, (SKT_ERROR)errno);
    return std::pair(res, SKT_ERROR::NONE);
  }

  std::pair<const void*, size_t> nativeSocket::getNative() const {
    return std::pair(&impl->skt, sizeof(int));
  }

  SKT_ERROR nativeSocket::close() {
    if (::close(impl->skt) < 0)
      return (SKT_ERROR)errno;
    return SKT_ERROR::NONE;
  }

  SKT_ERROR nativeSocket::shutdown(SD_HOW how) {
    if (::shutdown(impl->skt, (int)how) < 0)
      return (SKT_ERROR)errno;
    return SKT_ERROR::NONE;
  }

}

#endif