// vim:ts=2:sw=2:et
//------------------------------------------------------------------------------
/// \file  reactor_misc.cpp
//------------------------------------------------------------------------------
/// \brief Miscellaneous Reactor I/O utility functions — implementation
//------------------------------------------------------------------------------
// Copyright (c) 2015 Serge Aleynikov <saleyn@gmail.com>
// Created: 2015-03-10
//------------------------------------------------------------------------------
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <reactor/reactor_misc.hpp>
#include <reactor/reactor_platform.hpp>
#include <regex>
#include <sstream>
#include <sys/un.h>

namespace utxx {

//------------------------------------------------------------------------------
std::string EPollEvents(int a_events)
{
  std::stringstream s;
  int               n = 0;
  if (a_events & EPOLLIN) {
    if (n++) s << '|';
    s << "IN";
  }
  if (a_events & EPOLLPRI) {
    if (n++) s << '|';
    s << "PRI";
  }
  if (a_events & EPOLLOUT) {
    if (n++) s << '|';
    s << "OUT";
  }
  if (a_events & EPOLLRDNORM) {
    if (n++) s << '|';
    s << "RDNORM";
  }
  if (a_events & EPOLLRDBAND) {
    if (n++) s << '|';
    s << "RDBAND";
  }
  if (a_events & EPOLLWRNORM) {
    if (n++) s << '|';
    s << "WRNORM";
  }
  if (a_events & EPOLLWRBAND) {
    if (n++) s << '|';
    s << "WRBAND";
  }
  if (a_events & EPOLLMSG) {
    if (n++) s << '|';
    s << "MSG";
  }
  if (a_events & EPOLLERR) {
    if (n++) s << '|';
    s << "ERR";
  }
  if (a_events & EPOLLHUP) {
    if (n++) s << '|';
    s << "HUP";
  }
  if (a_events & EPOLLRDHUP) {
    if (n++) s << '|';
    s << "RDHUP";
  }
  if (a_events & EPOLLWAKEUP) {
    if (n++) s << '|';
    s << "WAKEUP";
  }
  if (a_events & EPOLLONESHOT) {
    if (n++) s << '|';
    s << "ONESHOT";
  }
  if (a_events & EPOLLET) {
    if (n++) s << '|';
    s << "ET";
  }
  return s.str();
}

//------------------------------------------------------------------------------
void GetIfAddr(char const* a_ip, std::string& a_ifname, std::string& a_ifip)
{
  REACTOR_PRETTY_FUNCTION();

  char buf[256], obuf[256];
  snprintf(buf, sizeof(buf), "ip -o -4 route get %s", a_ip);

  auto                                     deleter = [](FILE* p) { fclose(p); };
  std::unique_ptr<FILE, decltype(deleter)> file(::popen(buf, "r"), deleter);
  if (!file) REACTOR_THROWX_IO_ERROR(errno, "ip route get");
  if (!fgets(obuf, sizeof(obuf), file.get()))
    REACTOR_THROWX_IO_ERROR(errno, "fgets('ip -o -4 route get')");

  static const std::regex re1("^multicast +[^v]+via[^d]+dev +([^ ]+) src +([^ ]+)");
  static const std::regex re2("^multicast +[^d]+dev +([^ ]+) +src +([^ ]+)");
  static const std::regex re3("^[^d]+dev +([^ ]+) +src +([^ ]+)");

  const std::string       sbuf(obuf);
  std::smatch             sm;

  if (std::regex_search(sbuf, sm, re1)) {
    a_ifname = sm[1].str();
    a_ifip   = sm[2].str();
  } else if (std::regex_search(sbuf, sm, re2)) {
    a_ifname = sm[1].str();
    a_ifip   = sm[2].str();
  } else if (std::regex_search(sbuf, sm, re3)) {
    a_ifname = sm[1].str();
    a_ifip   = sm[2].str();
  } else
    REACTOR_THROWX_IO_ERROR(0, "'", a_ip, "' couldn't parse output of \"", buf, "\": ", obuf);
}

//------------------------------------------------------------------------------
in_addr GetIfAddr(std::string const& a_ifname)
{
  static const in_addr s_empty{0};

  int                  fd = socket(AF_INET, SOCK_DGRAM, 0);

  struct ifreq         ifr;
  ifr.ifr_addr.sa_family = AF_INET;
  strncpy(ifr.ifr_name, a_ifname.c_str(), IFNAMSIZ - 1);

  auto rc = ioctl(fd, SIOCGIFADDR, &ifr);
  close(fd);

  return rc < 0 ? s_empty : ((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr;
}

//------------------------------------------------------------------------------
std::string GetIfName(in_addr a_ifaddr)
{
  struct ifaddrs* addrs;
  getifaddrs(&addrs);

  auto guard = utxx::scope_exit([addrs]() { freeifaddrs(addrs); });

  for (auto iap = addrs; iap; iap = iap->ifa_next) {
    if (iap->ifa_addr && (iap->ifa_flags & IFF_UP) && iap->ifa_addr->sa_family == AF_INET) {
      auto sa = (struct sockaddr_in*)iap->ifa_addr;
      if (sa->sin_addr.s_addr == a_ifaddr.s_addr) {
        char buf[32];
        inet_ntop(iap->ifa_addr->sa_family, (void*)&sa->sin_addr, buf, sizeof(buf));
        return buf;
      }
    }
  }

  return std::string();
}

//------------------------------------------------------------------------------
bool Blocking(int a_fd, bool a_block)
{
  REACTOR_PRETTY_FUNCTION();

  int v = fcntl(a_fd, F_GETFL, 0);
  if (v < 0) REACTOR_THROWX_IO_ERROR(errno, "fcntl(F_GETFL)");

  int newv = a_block ? (v & ~O_NONBLOCK) : (v | O_NONBLOCK);

  if (fcntl(a_fd, F_SETFL, newv) < 0) REACTOR_THROWX_IO_ERROR(errno, "fcntl(F_SETFL)");

  return !(newv & O_NONBLOCK);
}

//------------------------------------------------------------------------------
bool IsUDSAlive(const std::string& a_uds_filename)
{
  static const size_t s_max_sz    = sizeof(sockaddr_un::sun_path);
  struct sockaddr_un  server_unix = {0};
  server_unix.sun_family          = AF_UNIX;
  strncpy(server_unix.sun_path, a_uds_filename.c_str(), s_max_sz);
  server_unix.sun_path[s_max_sz - 1] = '\0';

  int fd                             = socket(PF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) REACTOR_THROWX_IO_ERROR(errno, "socket()");

  auto guard = utxx::scope_exit([fd]() { (void)close(fd); });

  return ::connect(fd, (struct sockaddr*)&server_unix, sizeof(server_unix)) != -1;
}

} // namespace utxx
