// vim:ts=2:sw=2:et
//==============================================================================
/// @file      ReactorMisc.hpp
//------------------------------------------------------------------------------
/// @brief     Miscelaneous I/O utility functions.
//------------------------------------------------------------------------------
/// @copyright Copyright (c) 2015 Omnibius, LLC. All rights reserved.
/// @author    Serge Aleynikov
/// @date      2015-03-10
/// @license   Property of Omnibius licensed under terms found in LICENSE.txt
//------------------------------------------------------------------------------
//  Unauthorized copying of or deriving from this file or any part hereof, via
//  any medium is strictly prohibited by applicable law.
//
//  This source code is proprietary and confidential property.
//==============================================================================

#include <utxx/io/ReactorMisc.hpp>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <regex>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/constants.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/split.hpp>
#include <utxx/scope_exit.hpp>
#include <utxx/error.hpp>
#include <utxx/string.hpp>

using namespace std;

namespace utxx {
namespace io   {

//------------------------------------------------------------------------------
std::string EPollEvents(int a_events)
{
  std::stringstream s;
  int n = 0;
  if (a_events & EPOLLIN      ) { if (n++) s << '|'; s << "IN";      }
  if (a_events & EPOLLPRI     ) { if (n++) s << '|'; s << "PRI";     }
  if (a_events & EPOLLOUT     ) { if (n++) s << '|'; s << "OUT";     }
  if (a_events & EPOLLRDNORM  ) { if (n++) s << '|'; s << "RDNORM";  }
  if (a_events & EPOLLRDBAND  ) { if (n++) s << '|'; s << "RDBAND";  }
  if (a_events & EPOLLWRNORM  ) { if (n++) s << '|'; s << "WRNORM";  }
  if (a_events & EPOLLWRBAND  ) { if (n++) s << '|'; s << "WRBAND";  }
  if (a_events & EPOLLMSG     ) { if (n++) s << '|'; s << "MSG";     }
  if (a_events & EPOLLERR     ) { if (n++) s << '|'; s << "ERR";     }
  if (a_events & EPOLLHUP     ) { if (n++) s << '|'; s << "HUP";     }
  if (a_events & EPOLLRDHUP   ) { if (n++) s << '|'; s << "RDHUP";   }
  if (a_events & EPOLLWAKEUP  ) { if (n++) s << '|'; s << "WAKEUP";  }
  if (a_events & EPOLLONESHOT ) { if (n++) s << '|'; s << "ONESHOT"; }
  if (a_events & EPOLLET      ) { if (n++) s << '|'; s << "ET";      }
  return s.str();
}

//------------------------------------------------------------------------------
void GetIfAddr(char const* a_ip, std::string& a_ifname, std::string& a_ifip)
{
  UTXX_PRETTY_FUNCTION(); // Cache pretty function name

  char buf[256], obuf[256];
  snprintf(buf, sizeof(buf), "ip -o -4 route get %s", a_ip);
  {
    auto deleter = [&](FILE* p) { fclose(p); };
    std::unique_ptr<FILE, decltype(deleter)> file(::popen(buf, "r"), deleter);
    if (!file)
      UTXX_THROWX_IO_ERROR(errno, "ip route get");
    if (!fgets(obuf, sizeof(obuf), file.get()))
      UTXX_THROWX_IO_ERROR(errno, "fgets('ip -o -4 route get')");
  }

  static const std::regex re1
    ("^multicast +[^v]+via[^d]+dev +([^ ]+) src +([^ ]+)");
  static const std::regex re2
    ("^multicast +[^d]+dev +([^ ]+) +src +([^ ]+)");
  static const std::regex re3
    ("^[^d]+dev +([^ ]+) +src +([^ ]+)");
  const std::string sbuf(obuf);
  std::smatch sm;

  if (std::regex_search(sbuf, sm, re1)) {
    a_ifname = sm[1].str(); // dev
    a_ifip   = sm[2].str(); // src
  } else if (std::regex_search(sbuf, sm, re2)) {
    a_ifname = sm[1].str(); // dev
    a_ifip   = sm[2].str(); // src
  } else if (std::regex_search(sbuf, sm, re3)) {
    a_ifname = sm[1].str(); // dev
    a_ifip   = sm[2].str(); // src
  } else UTXX_THROWX_RUNTIME_ERROR
    ("'", a_ip, "' couldn't parse output of \"", buf, "\": ", obuf);
}

//------------------------------------------------------------------------------
in_addr GetIfAddr(std::string const& a_ifname)
{
  static const in_addr s_empty{0};

  auto fd = socket(AF_INET, SOCK_DGRAM, 0);

  struct ifreq ifr;
  ifr.ifr_addr.sa_family = AF_INET; // get an IPv4 IP address

  /* I want IP address attached to "eth0" */
  strncpy(ifr.ifr_name, a_ifname.c_str(), IFNAMSIZ-1);

  auto rc = ioctl(fd, SIOCGIFADDR, &ifr);

  close(fd);

  return rc < 0 ? s_empty : ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
}

//------------------------------------------------------------------------------
std::string GetIfName(in_addr a_ifaddr)
{
  struct ifaddrs *addrs;

  getifaddrs(&addrs);

  UTXX_SCOPE_EXIT([=]() { freeifaddrs(addrs); });

  for (auto iap = addrs; iap; iap = iap->ifa_next) {
    if (iap->ifa_addr && (iap->ifa_flags & IFF_UP) &&
        iap->ifa_addr->sa_family == AF_INET)
    {
      auto sa = (struct sockaddr_in *)(iap->ifa_addr);
      if  (sa->sin_addr.s_addr == a_ifaddr.s_addr) {
        char buf[32];
        inet_ntop(iap->ifa_addr->sa_family, (void *)&(sa->sin_addr), buf, sizeof(buf));
        return buf;
      }
    }
  }

  return std::string();
}

//------------------------------------------------------------------------------
bool Blocking(int a_fd, bool a_block)
{
  UTXX_PRETTY_FUNCTION(); // Cache pretty function name

  int v = fcntl(a_fd, F_GETFL, 0);
  if (v < 0)
    UTXX_THROWX_IO_ERROR(errno, "fcntl(F_GETFL)");

  int newv = a_block ? (v & (~O_NONBLOCK)) : (v|O_NONBLOCK);

  if (fcntl(a_fd, F_SETFL, newv) < 0)
    UTXX_THROWX_IO_ERROR(errno, "fcntl(F_SETFL)");

  return !(newv & O_NONBLOCK);
}

//------------------------------------------------------------------------------
bool IsUDSAlive(const string& a_uds_filename)
{
  static const size_t s_max_sz = sizeof(sockaddr_un::sun_path);
  struct sockaddr_un server_unix = { 0 };
  server_unix.sun_family         = AF_UNIX;
  strncpy(server_unix.sun_path, a_uds_filename.c_str(), s_max_sz);
  server_unix.sun_path[s_max_sz-1] = '\0';
  auto addr = (struct sockaddr*)&server_unix;
  auto size = sizeof(server_unix);

  int fd = socket(PF_UNIX, SOCK_STREAM, 0);

  if (fd < 0)
    UTXX_THROW_IO_ERROR(errno, "socket()");

  UTXX_SCOPE_EXIT([=]() { (void)close(fd); });

  int rc = ::connect(fd, addr, size);

  return rc != -1;
}

} // namespace io
} // namespace utxx
