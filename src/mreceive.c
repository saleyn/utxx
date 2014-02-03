/* vim: ts=2 sw=2 
 *
 * This program can be used to test presence of multicast traffic
 * and monitor its incoming rate.
 *
 * Copyright (c) 2014 Serge Aleynikov
 * Created: 2014-01-27
 * License: BSD open source
 */

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

typedef enum {UNDEFINED = -1, FORTS = 'f', MICEX = 'm'} data_fmt_t;

struct address {
  char                  iface_name[64];
  in_addr_t             iface;
  in_addr_t             mcast_addr;
  in_addr_t             src_addr;
  int                   port;
  int                   fd;
  data_fmt_t            data_format;    /* (m)icex, (f)orts */
  int                   last_seqno;
  int                   seq_gap_count;
  int                   ooo_count;      /* out-of-order packet count */
  struct epoll_event    event;
};

sigjmp_buf  jbuf;
struct address  addrs[128];
struct address* addrs_idx[1024]; // Maps fd -> addrs*
int         addrs_count = 0;
int         verbose     = 0;
int         terminate   = 0;
long        interval    = 5;
int         quiet       = 0;
long        start_time, next_time, now_time, last_time, pkt_time;
long        min_pkt_time=LONG_MAX, max_pkt_time=0, sum_pkt_time=0;
long        pkt_time_count=0, pkt_ooo_count=0;
long        tot_ooo_count = 0, tot_seq_gap_count = 0;
long        ooo_count   = 0, seq_gap_count = 0;
long        tot_bytes   = 0, tot_pkts      = 0, max_pkts = LONG_MAX;
int         last_bytes  = 0, bytes         = 0, pkts     = 0, last_pkts = 0;

void usage(const char* program) {
  printf("Listen to multicast traffic from a given (source addr) address:port\n\n"
         "Usage: %s [-c ConfigAddrs]\n"
         "          [-a Addr] [-m Mcastaddr -p Port [-s SourceAddr]] [-v] [-q] [-e]\n"
         "          [-i ReportingIntervalSec] [-d DurationSec] [-b RecvBufSize]\n"
         "          [-l ReportingLabel] [-o OutputFile]\n\n"
         "      -c CfgAddrs - Filename containing list of addresses to process\n"
         "      -a Addr     - Optional interface address or multicast address\n"
         "                    in the form: [MARKET+]udp://SrcIp@McastIp:Port\n"
         "                             or: [MARKET+]udp://SrcIp@McastIp;IfAddr:Port\n"
         "                    The MARKET label determines data format. Currently\n"
         "                    supported values are:\n"
         "                          micex, forts\n"
         "                    If interface address is not provided, it'll be\n"
         "                    determined automatically by a call to\n"
         "                       'ip route get...'\n"
         "      -e          - Use epoll()\n"
         "      -b Size     - Socket receive buffer size\n"
         "      -i Sec      - Reporting interval (default: 5s)\n"
         "      -d Sec      - Execution time in sec (default: infinity)\n"
         "      -l Label    - Title to include in the output report\n"
         "      -v          - Verbose (use -vv for more detailed output\n"
         "      -m MaxCount - Terminate after receiving this number of packets\n"
         "      -q          - Quiet (no output)\n\n"
         "      -o Filename - Output log file\n\n"
         "If there is no incoming data, press several Ctrl-C to break\n\n"
         "Return code: = 0  - if the process received at least one packet\n"
         "             > 0  - if no packets were received or there was an error\n\n"
         "Example:\n"
         "  %s -a micex+udp://91.203.253.233@239.195.4.11:26011 -v -i 1 -d 3\n\n",
         program, program);
  exit(1);
}

void sig_handler(int sig) {
  terminate++;
  if (sig == SIGALRM)
    siglongjmp(jbuf, 1);
  else if (terminate > 1) {
    fprintf(stderr, "Aboring...\n");
    exit(1);
  }
}

int non_blocking(int sfd) {
  int flags = fcntl(sfd, F_GETFL, 0);
  if (flags == -1) {
    perror ("fcntl(F_GETFL)");
    return -1;
  }

  if (fcntl(sfd, F_SETFL, flags | O_NONBLOCK) < 0) {
    perror ("fcntl(F_SETFL)");
    return -1;
  }

  if (verbose > 2)
    printf("Socket %d set to non-blocking mode\n", sfd);

  return 0;
}

char* data_fmt_string(data_fmt_t fmt, char* pfx, char* sfx, char* def) {
  static char buf[128];
  int n = sprintf(buf, "%s", pfx);
  switch (fmt) {
    case MICEX: n += sprintf(buf+n, "MICEX"); break;
    case FORTS: n += sprintf(buf+n, "FORTS"); break;
    default:
      sprintf(buf, def);
      return buf;
  }
  sprintf(buf+n, sfx);
  return buf;
}

void process_packet(struct address* addr, const char* buf, int n);

double scale(long n, long multiplier) {
  long g = multiplier*multiplier*multiplier;
  long m = multiplier*multiplier;
  long k = multiplier;
  double r = n;
  return n > g ? r/g :
         n > m ? r/m :
         n > k ? r/k :
         r;
}

const char* scale_suffix(long n, long multiplier) {
  long g = multiplier*multiplier*multiplier;
  long m = multiplier*multiplier;
  long k = multiplier;
  return n > g ? (multiplier == 1000 ? "B" : "G") :
         n > m ? "M" :
         n > k ? "K" :
         " ";
}

uint32_t decode_forts_seqno(const char* buf, int n, long last_seqno, int* seq_reset);

uint32_t get_seqno(data_fmt_t fmt, const char* buf, int n, long last_seqno, int* seq_reset) {
  switch (fmt) {
    case MICEX: *seq_reset = 0;
                return htonl(*(uint32_t*)buf);
    case FORTS: return decode_forts_seqno(buf, n, last_seqno, seq_reset);
  }
  return 0;
}

void inc_addrs() {
  if (++addrs_count == sizeof(addrs)/sizeof(addrs[0])) {
    fprintf(stderr, "Too many addresses provided (max=%d)\n",
      sizeof(addrs)/sizeof(addrs[0]));
    exit(1);
  }
}

void parse_addr(const char* s) {
  const char*   pif, *q;
  char          a[512];
  char*         addr        =  addrs[addrs_count].iface_name;
  in_addr_t*    iface       = &addrs[addrs_count].iface;
  in_addr_t*    mcast_addr  = &addrs[addrs_count].mcast_addr;
  in_addr_t*    src_addr    = &addrs[addrs_count].src_addr;
  int*          port        = &addrs[addrs_count].port;
  data_fmt_t*   data_format = &addrs[addrs_count].data_format;
  addrs[addrs_count].fd = -1;
  addrs[addrs_count].last_seqno = 0;
  addrs[addrs_count].seq_gap_count = 0;

  *addr         = '\0';
  *iface        = INADDR_NONE;
  *mcast_addr   = INADDR_NONE;
  *src_addr     = INADDR_NONE;
  *port         = -1;
  *data_format  = UNDEFINED;

  snprintf(a, sizeof(a), "%s", s);
  char* label = strchr(s, '+');
  if (label) {
    data_fmt_t code = (data_fmt_t)s[0];
    switch (code) {
      case MICEX: *data_format = MICEX; break;
      case FORTS: *data_format = FORTS; break;
      default:
        fprintf(stderr, "Invalid data format '%c' in: %s\n", (char)code, a);
        exit(1);
    }
    s = label+1;
  }
  if (verbose > 2)
    printf("Address: %s\n", s);
  if (strncmp(s, "udp://", 6)) {
    strncpy(addr, s, 64);
  } else {
    s += 6;
    char* p = strchr(s, '@');
    if (p) {
      *p++ = '\0';
      *src_addr = inet_addr(s);
      if (verbose > 2) printf("  %d: src=%s\n", addrs_count, s);
      s = p;
    }
    q = s;
    pif = p = strchr(s, ';');
    if (p) {
      *p++ = '\0';
      pif = p;
      s = p;
    }
    p = strchr(s, ':');
    if (!p) {
      fprintf(stderr, "Invalid multicast address (-a) specified: %s", a);
      exit(1);
    }
    *p++ = '\0';
    if (pif)
      strncpy(addr, pif, 64);
    *mcast_addr = inet_addr(q);
    *port = atoi(p);
    if (verbose > 2) {
      printf("  %d: mcast=%s port=%d iface=%s",
        addrs_count, q, *port, pif ? pif : "any");
    }

    if (verbose > 1) {
      printf("Adding iface=%s, mcast=%x, src=%x, port=%d\n",
        *addr ? addr : "any",
        addrs[addrs_count].mcast_addr,
        addrs[addrs_count].src_addr,
        addrs[addrs_count].port);
    }
    inc_addrs();
  }
}

void main(int argc, char *argv[])
{
  struct ip_mreq        group;
  struct ip_mreq_source group_s;
  struct epoll_event    events[256];
  const char*           label = NULL;
  int                   bsize = 0;
  struct timeval        tv;
  int                   use_epoll = 0, efd = -1;
  const char*           output_file = NULL;

  char*                 iaddr       = NULL;
  char*                 imcast_addr = NULL;
  char*                 isrc_addr   = NULL;
  int                   iport       = 0;
  int                   i;

  signal(SIGINT,  sig_handler);
  signal(SIGALRM, sig_handler);

  if (argc < 3)
    usage(argv[0]);

  memset(addrs_idx, 0, sizeof(addrs_idx));

  /* Parse command-line arguments */

  for (i=1; i < argc; ++i) {

    if (!strcmp(argv[i], "-c") && i < argc-1) {
      FILE* file = fopen(argv[++i], "r");
      char buf[512];
      if (!file) {
        fprintf(stderr, "Error opening config file '%s': %s\n",
                argv[i], strerror(errno));
        exit(1);
      }
      while (fgets(buf, sizeof(buf), file)) {
        char* p = buf;
        while(*p == ' ' || *p == '\t') p++;
        if (strlen(p) && *p != '#') {
          if (verbose > 1)
            printf("Parsing address: %s\n", p);
          parse_addr(p);
        }
      }
      fclose(file);
    } else if (!strcmp(argv[i], "-a") && i < argc-1)
      parse_addr(argv[++i]);
    else if (!strcmp(argv[i], "-p") && i < argc-1)
      iport = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-m") && i < argc-1)
      imcast_addr = argv[++i];
    else if (!strcmp(argv[i], "-s") && i < argc-1)
      isrc_addr = argv[++i];
    else if (!strcmp(argv[i], "-b") && i < argc-1)
      bsize = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-i") && i < argc-1)
      interval = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-m") && i < argc-1)
      max_pkts = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-o") && i < argc-1)
      output_file = argv[++i];
    else if (!strcmp(argv[i], "-d") && i < argc-1) {
      gettimeofday(&tv, NULL);
      alarm(atoi(argv[++i]));
    } else if (!strncmp(argv[i], "-v", 2))
      verbose += strlen(argv[i])-1;
    else if (!strncmp(argv[i], "-e", 2))
      use_epoll = 1;
    else if (!strncmp(argv[i], "-q", 2))
      quiet = 1;
    else if (!strcmp(argv[i], "-l") && i < argc-1)
      label = argv[++i];
    else
      usage(argv[0]);
  }

  if (!addrs_count) {
    if (!imcast_addr || !iport)
      usage(argv[0]);
    addrs[addrs_count].mcast_addr = inet_addr(imcast_addr);
    addrs[addrs_count].src_addr   = inet_addr(isrc_addr);
    addrs[addrs_count].port       = iport;
    addrs[addrs_count].fd         = -1;

    inc_addrs();
  }

  if (output_file) {
    fflush(stdout);
    efd = open(output_file, O_APPEND | O_WRONLY);
    if (efd < 0) {
      fprintf(stderr, "Cannot open file '%s' for writing: %s\n",
        output_file, strerror(errno));
      exit(1);
    }
    dup2(efd, 1);
    close(efd);
  }

  if (use_epoll) {
    efd = epoll_create1(0);
    if (efd < 0) {
      perror("epoll_create1");
      exit(1);
    }
  } else if (addrs_count > 1) {
    fprintf(stderr, "Cannot specify more than one address without using epoll (-e)!");
    exit(1);
  }

  /* Initialize all sockets */
  for (i=0; i < addrs_count; i++) {
    if (addrs[i].mcast_addr == INADDR_NONE || addrs[i].port < 0) {
      fprintf(stderr, "Invalid mcast address or port specified (addr #%d of %d): %d, %d\n",
        i+1, addrs_count, addrs[i].mcast_addr, addrs[i].port);
      exit(1);
    }

    /* Create a datagram socket on which to receive. */
    addrs[i].fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (addrs[i].fd < 0) {
        perror("opening datagram socket");
        exit(1);
    } else if (verbose > 1) {
      printf(" Addr#%d opened fd = %d\n", i, addrs[i].fd);
    }

    assert(addrs[i].fd < sizeof(addrs_idx)/sizeof(addrs_idx[0]));
    addrs[i].event.data.fd = addrs[i].fd;
    addrs[i].event.events  = EPOLLIN | EPOLLET; // Edge-triggered
    addrs_idx[addrs[i].fd] = addrs + i;

    if (use_epoll) {
      if (epoll_ctl(efd, EPOLL_CTL_ADD, addrs[i].fd, &addrs[i].event) < 0) {
        perror("epoll_ctl(add)");
        exit(1);
      }
    }

    /*
    * Enable SO_REUSEADDR to allow multiple instances of this
    * application to receive copies of the multicast datagrams.
    */
    {
      int reuse=1;
      if (setsockopt(addrs[i].fd, SOL_SOCKET, SO_REUSEADDR,
                    (char *)&reuse, sizeof(reuse)) < 0) {
        perror("setting SO_REUSEADDR");
        close(addrs[i].fd);
        exit(1);
      }
    }

    /* Set receive buffer size */
    if (bsize && setsockopt(addrs[i].fd, SOL_SOCKET, SO_RCVBUF, &bsize, sizeof(bsize))) {
        perror("setting SO_RCVBUF");
        exit(1);
    }

    /* Figure out which network interface to use */
    if (!addrs[i].iface_name) {
      addrs[i].iface = INADDR_ANY;
      if (verbose > 1)
        printf("Using INADDR_ANY interface\n");
    } else if (strlen(addrs[i].iface_name) == 0) {
      char buf[256], obuf[256], mc[32], via[32], src[32];
      FILE* file;
      snprintf(buf, sizeof(buf), "ip -o -4 route get %s",
        inet_ntoa(*(struct in_addr*)&addrs[i].mcast_addr));
      file = popen(buf, "r");
      if (!file) {
        perror("ip route get");
        exit(1);
      }
      if (!fgets(obuf, sizeof(obuf), file)) {
        perror("ip route get | fgets()");
        exit(1);
      }
      fclose(file);
      if (verbose > 1)
        printf("Executed: '%s' ->\n  %s\n", buf, obuf);
      if (sscanf(obuf, "multicast %16s via %16s dev %16s src %16s ",
          mc, via, addrs[i].iface_name, src) != 4) {
        fprintf(stderr, "Couldn't parse output of 'ip route get'\n");
        exit(2);
      }
      if (inet_aton(src, (struct in_addr*)&addrs[i].iface) == 0) {
        fprintf(stderr, "Cannot parse address of interface '%s' %s\n",
          addrs[i].iface_name, src);
        exit(3);
      }
    } else {
      struct ifreq ifr;
      ifr.ifr_addr.sa_family = AF_INET;
      strncpy(ifr.ifr_name, addrs[i].iface_name, IFNAMSIZ-1);


      if (ioctl(addrs[i].fd, SIOCGIFADDR, &ifr) >= 0) {
        addrs[i].iface = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
        if (verbose)
          printf("Looked up interface '%s' address: %s\n",
            addrs[i].iface_name,
            inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
      } else {
        int e = errno;
        if (inet_aton(addrs[i].iface_name, (struct in_addr*)&addrs[i].iface) == 0) {
          fprintf(stderr, "Can't get interface '%s' address: %s\n",
            addrs[i].iface_name, strerror(e));
          exit(1);
        }

        if (verbose)
          printf("Using %s interface (%x)\n", addrs[i].iface_name, addrs[i].iface);
      }

    }

    /* Bind to the proper port number with the IP address */
    {
      struct sockaddr_in local_s;
      memset((char *) &local_s, 0, sizeof(local_s));
      local_s.sin_family        = AF_INET;
      local_s.sin_port          = htons(addrs[i].port);
      local_s.sin_addr.s_addr   = INADDR_ANY;

      if (bind(addrs[i].fd, (struct sockaddr*)&local_s, sizeof(local_s))) {
          perror("binding datagram socket");
          exit(1);
      }
    }

    /*
    * Join the (source-specific?) multicast group on the given interface.
    * Note that this IP_ADD_(SOURCE_)MEMBERSHIP option must be
    * called for each local interface over which the multicast
    * datagrams are to be received.
    */

    if (!quiet && verbose) {
      char iface[32], mcast[32], src[32];
      struct in_addr in;
      snprintf(iface, sizeof(iface), "%s", inet_ntoa(*((struct in_addr*)&addrs[i].iface)));
      snprintf(mcast, sizeof(mcast), "%s", inet_ntoa(*((struct in_addr*)&addrs[i].mcast_addr)));
      snprintf(src,   sizeof(src),   "%s", inet_ntoa(*((struct in_addr*)&addrs[i].src_addr)));

      printf("Joining %smulticast %s%s%s on interface %s:%d%s\n",
        addrs[i].src_addr != INADDR_NONE ? "source-specific " : "",
        addrs[i].src_addr != INADDR_NONE ? src : "",
        addrs[i].src_addr ? "@" : "",
        mcast, iface, addrs[i].port,
        data_fmt_string(addrs[i].data_format, " (", ")", ""));
    }

    if (addrs[i].src_addr != INADDR_NONE) {
      group_s.imr_multiaddr.s_addr  = addrs[i].mcast_addr;
      group_s.imr_sourceaddr.s_addr = addrs[i].src_addr;
      group_s.imr_interface.s_addr  = addrs[i].iface;

      if (setsockopt(addrs[i].fd, IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP,
                  (char *)&group_s, sizeof(group_s)) < 0) {
        perror("adding source multicast group");
        exit(1);
      }
    } else {
      group.imr_multiaddr.s_addr = addrs[i].mcast_addr;
      group.imr_interface.s_addr = addrs[i].iface;
      if (setsockopt(addrs[i].fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                  (char *)&group, sizeof(group)) < 0) {
        perror("adding multicast group");
        exit(1);
      }
    }

    if (use_epoll)
      non_blocking(addrs[i].fd);
  }

  gettimeofday(&tv, NULL);
  start_time = now_time = last_time = tv.tv_sec * 1000000 + tv.tv_usec;
  next_time  = start_time + interval * 1000000;

  /*----------------------------------------------------------------------
   * Main data loop
   *--------------------------------------------------------------------*/

  srand(time(NULL));
  setjmp(jbuf);

  while (!terminate) {
    char databuf[16*1024];
    int events_count, n = -1, i;

    if (use_epoll) {
      if (verbose > 2) printf("  Calling epoll(%d)...\n", efd);
      events_count = epoll_wait(efd, events, sizeof(events)/sizeof(events[0]), -1);
      if (verbose > 2) printf("  epoll() -> %d\n", events_count);

      if (events_count < 0) {
        if (errno == EINTR)
          continue;
        perror("epoll_wait");
        exit(1);
      }
      n = 0;
    } else {
      int fd = addrs[0].fd;
      if (verbose > 2) printf("  Calling read(%d, size=%d)...\n", fd, sizeof(databuf));
      n = read(fd, databuf, sizeof(databuf));
      if (verbose > 2) printf("  Got %d bytes\n", n);
      events_count = 1;
      events[0].data.fd = fd;
      events[0].events  = EPOLLIN;
    }

    for (i=0; i < events_count; ++i) {
      struct address* addr = addrs_idx[events[i].data.fd];

      do {
        if (use_epoll)
          n = read(addr->fd, databuf, sizeof(databuf));
        if (n < 0) {
          if (errno != EAGAIN && errno != EINTR) {
            perror("read");
            terminate = 1;
            close(addr->fd);
          }
          break;
        }
        process_packet(addr, databuf, n);
      } while (use_epoll);
    }
  }

  /*----------------------------------------------------------------------
   * Print summary
   *--------------------------------------------------------------------*/
  gettimeofday(&tv, NULL);

  if (!quiet) {
    double sec = (double)(tv.tv_sec * 1000000 + tv.tv_usec - start_time)/1000000;
    if (sec == 0.0) sec = 1.0;
    printf("%-30s| %6.1f KB/s %6d pkts/s| %9ld %sB %9ld %spkts | OutOfSeq %d | Lost: %d\n",
      label ? label : "TOTAL",
      tot_bytes / 1024 / sec, (int)(tot_pkts / sec),
      (long)scale(tot_bytes, 1024), scale_suffix(tot_bytes, 1024),
      (long)scale(tot_pkts,  1000), scale_suffix(tot_pkts,  1000),
      tot_ooo_count, tot_seq_gap_count);
  }

  exit(tot_pkts ? 0 : 1);
}

void process_packet(struct address* addr, const char* buf, int n) {
  struct timeval tv;
  int seqno;

  gettimeofday(&tv, NULL);
  now_time = tv.tv_sec * 1000000 + tv.tv_usec;

  /* Get timestamp of the packet */
  if (last_pkts < 1000 && pkts < 1000 || (rand() % 100) < 10) {
    ioctl(addr->fd, SIOCGSTAMP, &tv);
    pkt_time = now_time - (tv.tv_sec * 1000000 + tv.tv_usec);
    sum_pkt_time += pkt_time;
    if (pkt_time < min_pkt_time) min_pkt_time = pkt_time;
    if (pkt_time > max_pkt_time) max_pkt_time = pkt_time;
    pkt_time_count++;
  }

  tot_bytes += n;
  tot_pkts++;
  bytes += n;
  pkts++;

  int seq_reset;
  seqno = get_seqno(addr->data_format, buf, n, addr->last_seqno, &seq_reset);
  if (seqno) {
    if (addr->last_seqno) {
      int diff = seqno - addr->last_seqno;
      if (!seq_reset) {
        if (diff < 0) {
          if (verbose > 1)
            printf("  Out of order seqno (last=%ld, now=%ld): %d\n",
              addr->last_seqno, seqno, diff);
          addr->ooo_count++;
          tot_ooo_count++;  /* out of order */
          ooo_count++;
        } else if (diff > 1) {
          addr->seq_gap_count++;
          tot_seq_gap_count++;
          seq_gap_count++;
          if (verbose > 1)
            printf("  Gap detected in seqno (last=%ld, now=%ld): %d\n",
              addr->last_seqno, seqno, diff);
        }
      }
    }
    addr->last_seqno = seqno;
  }

  if (pkts >= max_pkts)
    terminate = 1;

  if (verbose > 2)
    printf("Received %6d bytes, %ld packets\n", n, tot_pkts);

  if (!quiet && (interval || verbose) && now_time >= next_time) {
    double sec = (double)(now_time - last_time)/1000000;
    struct tm* tm  = localtime(&tv.tv_sec);
    double avg_lat = pkt_time_count ? (double)sum_pkt_time / pkt_time_count : 0.0;
    if (sec == 0.0) sec = 1.0;
    printf("%02d:%02d:%02d|%6.1f K/s %6d p/s|o: %4d %s|gap: %4d %s|"
           "Tot: %5d %s %6d %sp|o: %4d %s|gap: %4d %s|"
           "Lat:%6d @ %6.1fus (%d..%d)\n",
        tm->tm_hour, tm->tm_min, tm->tm_sec,
        scale(bytes, 1024) / sec, (int)(pkts / sec),
        (int)scale(ooo_count,         1000), scale_suffix(ooo_count,        1000),
        (int)scale(seq_gap_count,     1000), scale_suffix(seq_gap_count,    1000),
        (int)scale(tot_bytes,         1024), scale_suffix(tot_bytes,        1024),
        (int)scale(tot_pkts,          1000), scale_suffix(tot_pkts,         1000),
        (int)scale(tot_ooo_count,     1000), scale_suffix(tot_ooo_count,    1000),
        (int)scale(tot_seq_gap_count, 1000), scale_suffix(tot_seq_gap_count,1000),
        pkt_time_count, avg_lat,
        pkt_time_count ? min_pkt_time : 0,
        max_pkt_time);

    min_pkt_time  = LONG_MAX;
    max_pkt_time  = 0;
    sum_pkt_time  = 0;
    pkt_time_count= 0;
    last_pkts     = pkts;
    bytes = pkts  = 0;
    ooo_count     = 0;
    seq_gap_count = 0;
    last_time     = now_time;
    next_time     = now_time + interval*1000000;

    fflush(stdout);
  }
}

static inline int u32_to_size (uint32_t n)
{
  static const int s_table[] = {
    5, 5, 5, 5,
    4, 4, 4, 4, 4, 4, 4,
    3, 3, 3, 3, 3, 3, 3, 
    2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1
  };
  int first_bit = n ? 31 : __builtin_clz(n);
  return s_table[first_bit];
}

/*
uint32_t decode_uint32(const char** pbytes, const char* end)
{
  static const uint64_t s_mask = 0x8080808080808080;
  const uint64_t* pval = *(const uint64_t**)bytes;
  uint64_t stop = *pval & s_mask;
  int      bits = stop ? __builtin_clzl(stop) : -1;
  int     bytes = (bits >> 3) + 1;
  uint64_t  res = (*pval >> (56-bits)) & 0x7F7F7F7F7F7F7F7F;

  switch (bytes) {
    case 1: return res;
    case 2: return res       & 0x0000007F
                | (res >> 1) & 0x00003F80;
    case 3: return res       & 0x0000007F
                | (res >> 1) & 0x00003F80
                | (res >> 2) & 0x003Fc000;
    case 3: return res       & 0x0000007F
                | (res >> 1) & 0x00003F80
                | (res >> 2) & 0x003Fc000
                | (res >> 3) & 0x;
  assert(bits >= 0);

  *pbytes += (n >> 8);

  const uint8_t* p;

  if (0 == nbytes)  return 0;

}
*/

int decode_uint_loop(const char** buff, const char* end, uint64_t* val) {
  const char* p = *buff;
  int i;
  uint64_t n = 0;

  for (i=0; p != end; i += 7) {
    uint64_t m = (uint64_t)(*p & 0x7F) << i;
    n |= m;
    //printf("    i=%d, p=%02x, m=%016x, n=%016x\n", i, *(uint8_t*)p, m, n);
    if (*p++ & 0x80) {
        *val  = n;
        n     = p - *buff;
        *buff = p;
        return n;
    }
  }

  return 0;
}

int unmask_7bit_uint56(const char** buff, const char* end, uint64_t* value) {
    static const uint64_t s_mask   = 0x8080808080808080ul;
    static const uint64_t s_unmask = 0x7F7F7F7F7F7F7F7Ful;

    uint64_t v    = *(const uint64_t*)*buff;
    uint64_t stop = v & s_mask;
    if (stop) {
        int      pos  = __builtin_ffsl(stop);
        uint64_t shft = pos == 64 ? -1ul : (1ul << pos)-1;
        pos  >>= 3;
        *value = v & s_unmask & shft;
        *buff += pos;
        return pos;
    }

    *value = v & s_unmask;
    *buff += 8;
    return 0;
}

inline int find_stopbit_byte(const char** buff, const char* end) {
    const uint64_t s_mask = 0x8080808080808080ul;
    const char*     start = *buff;

    uint64_t v = *(const uint64_t*)start;
    uint64_t stop = v & s_mask;
    int pos;

    if (stop) {
        pos = __builtin_ffsl(stop) >> 3;
        *buff += pos;
    } else {
        // In case the stop bit is not found in 64 bits,
        // we need to check next bytes
        const char* p = *buff + 8;
        if (p > end) p = end;
        pos = find_stopbit_byte(&p, end);
        *buff = p;
    }

    return pos;
}


//-------------------------------------------------------------------------//
// Extracting "PMap" and TemplateID:                                       //
//-------------------------------------------------------------------------//
// NB: "PMap" bits are stored in the resulting "uint64" in the order of incr-
// easing significance, from 0 to 62 (bit 63 is unused).
// Also note: "tid" may be NULL, in which case we get only the "PMap" (useful
// for processing Sequences):
//
uint32_t decode_forts_seqno(const char* buff, int n, long last_seqno, int* seq_reset) {
    int res, len = find_stopbit_byte(&buff, buff+n);
    const char* q = buff;
    uint64_t  tid, seq = 0;

    res = decode_uint_loop(&q, q+5, &tid);

    if (tid == 120) // reset
      return last_seqno;

    res = decode_uint_loop(&q, q+5, &seq);

    // If sequence reset, parse new seqno
    if (tid == 49) {
      *seq_reset = 1;
      res = decode_uint_loop(&q, q+10, &tid); // SendingTime
      res = decode_uint_loop(&q, q+5,  &seq); // NewSeqNo
    } else if (last_seqno && abs(last_seqno - seq) > 1) {
      //printf("Seq gap (tid=%ld) last=%ld seq=%ld\n", tid, last_seqno, seq);
    }

    return seq;
}

