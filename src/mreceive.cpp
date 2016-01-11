/* vim: ts=2 sw=2 
 *
 * This program can be used to test presence of multicast traffic
 * and monitor its incoming rate.
 *
 * Here is a sample output it produces:
 *
 * #S|Sok:  21| KBytes/s|Pkts/s|OutOfO|SqGap|Es|Gs|Os|TOT|  MBytes| KPakets|OutOfOrd| TotGaps|Lat N  Avg Mn   Max|
 * II|16:49:45|    149.4|   864|     0|    0| 2| 0| 0|TOT|   451.5|    3626|       0|       0|  455  3.6  1    14|
 * II|16:49:50|    181.9|  1374|     0|    0| 2| 0| 0|TOT|   452.4|    3633|       0|       0|  694  3.3  1    12|
 * II|16:49:55|    134.4|  1004|     0|    0| 2| 0| 0|TOT|   453.1|    3638|       0|       0|  475  3.3  1    14|
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
#include <sys/timerfd.h>
#include <sys/types.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define unlikely(expr) __builtin_expect(!!(expr), 0)
#define likely(expr)   __builtin_expect(!!(expr), 1)

typedef enum {
  UNDEFINED = 0,
  FORTS     = 'f',
  MICEX     = 'm'
} data_fmt_t;

typedef enum {
  OK = 0,
  NODATA_OFF    =  1, NODATA_ON = 2,
  OOO_OFF       =  4, OOO_ON    = 8,
  GAP_OFF       = 16, GAP_ON    = 32
} src_state_t;

const int MEGABYTE = 1024*1024;
//const int MILLION  = 1000000;

struct address {
  int                   id;             /* url order in the config */
  char                  url[256];
  char*                 title;
  char                  iface_name[64];
  in_addr_t             iface;
  in_addr_t             mcast_addr;
  in_addr_t             src_addr;
  int                   port;
  int                   fd;
  data_fmt_t            data_format;    /* (m)icex, (f)orts */
  long                  last_data_time; /* time of the last gap detected */
  long                  last_seqno;
  long                  last_ooo_time;  /* time of the last gap detected */
  long                  last_gap_time;  /* time of the last gap detected */

  long                  bytes_cnt;      /* total byte count */
  int                   pkt_count;      /* total pkt count  */
  int                   gap_count;      /* number of lost packets (due to seq gaps) */
  int                   ooo_count;      /* out-of-order packet count */

  // Total summary reports
  int                   last_srep_pkt_count;
  int                   last_srep_ooo_count;
  int                   last_srep_gap_count;

  // Individual channel summary reports
  int                   last_crep_pkt_count;
  int                   last_crep_ooo_count;
  int                   last_crep_gap_count;
  int                   last_crep_pkt_changed;

  src_state_t           state;          /* state of gap detector */
  struct epoll_event    event;
};

sigjmp_buf  jbuf;
struct address   addrs[128];
struct address*  addrs_idx[1024];   // Maps fd -> addrs*
struct address** sorted_addrs[4];   // For report stats sorting

int         wfd           = -1;
const char* label         = NULL;
int         addrs_count   = 0;
int         verbose       = 0;
int         terminate     = 0;
long        interval      = 5;
long        sock_interval = 50;
int         quiet         = 0;
int         max_title_width = 0;
long        start_time, now_time, last_time, pkt_time;
long        min_pkt_time=LONG_MAX, max_pkt_time=0, sum_pkt_time=0;
long        pkt_time_count=0, pkt_ooo_count=0;
long        tot_ooo_count = 0, tot_gap_count = 0;
long        ooo_count     = 0, gap_count = 0;
long        tot_bytes     = 0, tot_pkts      = 0, max_pkts = LONG_MAX;
int         last_bytes    = 0, bytes         = 0, pkts     = 0, last_pkts = 0;
int         output_lines_count        = 0;
int         next_legend_count         = 1;
int         next_sock_report_lines    = 5;
int         max_channel_report_lines  = 10;
int         display_packets           = 0;
int         display_packets_hex       = 0;
const char* output_file               = NULL;
const char* write_file                = NULL;

void usage(const char* program) {
  printf("Listen to multicast traffic from a given (source addr) address:port\n\n"
         "Usage: %s [-c ConfigAddrs]\n"
         "          [-a Addr] [-n Mcastaddr -p Port [-s SourceAddr]] [-v] [-q] [-e false]\n"
         "          [-i ReportingIntervalSec] [-I SockReportInterval]\n"
         "          [-d DurationSec] [-b RecvBufSize] [-L MaxChannelReportLines]\n"
         "          [-l ReportingLabel] [-r PrintPacketSize] [-o OutputFile]\n\n"
         "      -c CfgAddrs - Filename containing list of addresses to process\n"
         "                    (use \"-\" for stdin)\n"
         "      -a Addr     - Optional interface address or multicast address\n"
         "                    in the form:\n"
         "                        [MARKET+]udp://SrcIp@McastIp[;IfAddr]:Port[/TITLE]\n"
         "                    The MARKET label determines data format. Currently\n"
         "                    supported values are:\n"
         "                          micex, forts\n"
         "                    If interface address is not provided, it'll be\n"
         "                    determined automatically by a call to\n"
         "                       'ip route get...'\n"
         "      -e false    - Don't use epoll() (default: true)\n"
         "      -b Size     - Socket receive buffer size\n"
         "      -i Sec      - Reporting interval (default: 5s)\n"
         "      -I Lines    - Socket reporting interval (default: 50)\n"
         "      -L Lines    - Max number of channel-level report lines (default: 10)\n"
         "      -d Sec      - Execution time in sec (default: infinity)\n"
         "      -l Label    - Title to include in the output report\n"
         "      -v          - Verbose (use -vv for more detailed output)\n"
         "      -n MaxCount - Terminate after receiving this number of packets\n"
         "      -P [Size]   - Print packet up to Size bytes in ASCII format\n"
         "      -X [Size]   - Print packet up to Size bytes in HEX format\n"
         "      -q          - Quiet (no output)\n"
         "      -o Filename - Output log file\n"
         "      -w Filename - Write packets to file\n\n"
         "If there is no incoming data, press several Ctrl-C to break\n\n"
         "Return code: = 0  - if the process received at least one packet\n"
         "             > 0  - if no packets were received or there was an error\n\n"
         "Example:\n"
         "  %s -a \"micex+udp://91.203.253.233@239.195.4.11:26011/RTS-5\" -v -i 1 -d 3\n\n"
         "Reporting format:\n"
         "  |Sok:|          - Socket ID\n"
         "  |KBytes/s|      - KBytes per second\n"
         "  |Pkts/s|        - Packets per second rate\n"
         "  |OutOfO|        - Out of order packets (available if MARKET is supported by this tool)\n"
         "  |SqGap|         - Number of sequence gaps (available if MARKET is supported)\n"
         "  |Es|            - Number of empty sockets\n"
         "  |Gs|            - Number of sockets that had gaps\n"
         "  |Os|            - Number of sockets that has out-of-order packets\n"
         "\n",
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
  static char buf[256];
  char* p = buf;
  p = stpcpy(p, pfx);
  switch (fmt) {
    case MICEX: p = stpcpy(p, "MICEX"); break;
    case FORTS: p = stpcpy(p, "FORTS"); break;
    default:
      stpcpy(p, def);
      return buf;
  }
  stpcpy(p, sfx);
  return buf;
}

void print_report();
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

void test_forts_decode();
uint32_t decode_forts_seqno(const char* buf, int n, long last_seqno, int* seq_reset);

long get_seqno(struct address* addr, const char* buf, int n, long last_seqno, int* seq_reset) {
  switch (addr->data_format) {
    case MICEX: {
      uint32_t a = (uint8_t)buf[0],
               b = (uint8_t)buf[1],
               c = (uint8_t)buf[2],
               d = (uint8_t)buf[3];
      return (uint32_t)d << 24 | (uint32_t)c << 16 | (uint32_t)b << 8 | a;
    }
    case FORTS:
      return decode_forts_seqno(buf, n, last_seqno, seq_reset);
    default:
      break;
  }
  return 0;
}

void inc_addrs() {
  if (++addrs_count == sizeof(addrs)/sizeof(addrs[0])) {
    fprintf(stderr, "Too many addresses provided (max=%lu)\n",
      sizeof(addrs)/sizeof(addrs[0]));
    exit(1);
  }
}

void parse_addr(char* s) {
  const char*       pif, *q;
  char              a[512];
  struct address*   paddr       = &addrs[addrs_count];
  char*             url         =  paddr->url;
  char**            title       = &paddr->title;
  char*             addr        =  paddr->iface_name;
  in_addr_t*        iface       = &paddr->iface;
  in_addr_t*        mcast_addr  = &paddr->mcast_addr;
  in_addr_t*        src_addr    = &paddr->src_addr;
  int*              port        = &paddr->port;
  data_fmt_t*       data_format = &paddr->data_format;

  memset(paddr, 0, sizeof(struct address));

  paddr->id                     = addrs_count;
  paddr->fd                     = -1;
  *iface                        = INADDR_NONE;
  *mcast_addr                   = INADDR_NONE;
  *src_addr                     = INADDR_NONE;
  *port                         = -1;
  paddr->last_crep_pkt_changed  = -1;

  snprintf(a,   sizeof(a), "%s", s);
  snprintf(url, sizeof(((struct address*)0)->url), "%s", s);

  if (verbose > 2)
    printf("Address: %s\n", s);

  {
    char* p;
    /* Remove the title from url */
    if ((p = strchr(url, ':')) && 
        (p = strchr(p+1, ':')) &&
        (p = strchr(p+1, '/'))) {
      a[p - url] = '\0';
      *p++ = '\0';
      while (*p == ' ') p++;
      *title = p;
      for (; *p; p++)
        if (*p == '\n') {
          *p = '\0';
          break;
        }
      int n = strlen(*title);
      if (n > max_title_width) max_title_width = n;
    }
  }

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
      snprintf(addr, sizeof(((struct address*)0)->iface_name), "%s", pif);
    *mcast_addr = inet_addr(q);
    *port = atoi(p);
    if (verbose > 2) {
      printf("  %d: mcast=%s port=%d iface=%s title='%s'\n",
        addrs_count, q, *port, pif ? pif : "any", *title);
    }

    if (verbose > 2) {
      printf("Adding iface=%s, mcast=%x, src=%x, port=%d\n",
        *addr ? addr : "any",
        paddr->mcast_addr,
        paddr->src_addr,
        paddr->port);
    }
    inc_addrs();
  }
}

int main(int argc, char *argv[])
{
  struct ip_mreq        group;
  struct ip_mreq_source group_s;
  struct epoll_event    events[256];
  int                   bsize = 0;
  struct timeval        tv;
  int                   use_epoll = 1, efd = -1, tfd = -1;
  struct epoll_event    timer_event;

  char*                 imcast_addr = NULL;
  char*                 isrc_addr   = NULL;
  int                   iport       = 0;
  int                   i;

  signal(SIGINT,  sig_handler);
  signal(SIGALRM, sig_handler);

  if (argc < 3)
    usage(argv[0]);

  memset(addrs_idx,    0, sizeof(addrs_idx));
  memset(sorted_addrs, 0, sizeof(sorted_addrs));

  /* Parse command-line arguments */

  for (i=1; i < argc; ++i)
    if (!strncmp(argv[i], "-v", 2))
      verbose += strlen(argv[i])-1;

  for (i=1; i < argc; ++i) {

    if (!strcmp(argv[i], "-c") && i < argc-1) {
      const char* cfgfile = argv[++i];
      FILE* file = strcmp("-", cfgfile) ? fopen(cfgfile, "r") : stdin;
      char buf[512];
      if (!file) {
        fprintf(stderr, "Error opening config file '%s': %s\n",
                argv[i], strerror(errno));
        exit(1);
      }
      while (fgets(buf, sizeof(buf), file)) {
        char* p = buf;
        while(*p == ' ' || *p == '\t') p++;
        if (strlen(p) && *p != '#')
          parse_addr(p);
      }
      if (fileno(file))
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
    else if (!strcmp(argv[i], "-I") && i < argc-1)
      next_sock_report_lines = sock_interval = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-L") && i < argc-1)
      max_channel_report_lines = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-n") && i < argc-1)
      max_pkts = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-o") && i < argc-1)
      output_file = argv[++i];
    else if (!strcmp(argv[i], "-w") && i < argc-1)
      write_file = argv[++i];
    else if (!strcmp(argv[i], "-d") && i < argc-1) {
      gettimeofday(&tv, NULL);
      alarm(atoi(argv[++i]));
    } else if (!strncmp(argv[i], "-v", 2))
      (void)0;
    else if (!strncmp(argv[i], "-e", 2)) {
      if (i < argc-1 && !strcmp(argv[i+1], "false")) {
        i++;
        use_epoll = 0;
      }
    } else if (!strncmp(argv[i], "-P", 2))
      display_packets = (i < argc-1) ? atoi(argv[++i]) : 512;
    else if (!strncmp(argv[i], "-X", 2)) {
      display_packets = (i < argc-1) ? atoi(argv[++i]) : 512;
      display_packets_hex = 1;
    } else if (!strncmp(argv[i], "-q", 2))
      quiet = 1;
    else if (!strcmp(argv[i], "-l") && i < argc-1)
      label = argv[++i];
    else
      usage(argv[0]);
  }

  /* No "-c" and "-a" options given: obtain address from other parameters */
  if (!addrs_count) {
    if (!imcast_addr || !iport)
      usage(argv[0]);
    addrs[addrs_count].mcast_addr = inet_addr(imcast_addr);
    addrs[addrs_count].src_addr   = inet_addr(isrc_addr);
    addrs[addrs_count].port       = iport;
    addrs[addrs_count].fd         = -1;

    inc_addrs();
  }

  /* Setup output */
  if (output_file) {
    fflush(stdout);
    efd = open(output_file, O_CREAT | O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP);
    if (efd < 0) {
      fprintf(stderr, "Cannot open file '%s' for writing: %s\n",
        output_file, strerror(errno));
      exit(1);
    }
    dup2(efd, 1);
    close(efd);
  }

  if (write_file) {
    wfd = open(write_file, O_CREAT | O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP);
    if (wfd < 0) {
      fprintf(stderr, "Cannot open file '%s' for writing: %s\n",
        write_file, strerror(errno));
      exit(1);
    }
  }

  if (addrs_count > 1 && !use_epoll) {
    if (verbose)
      printf("Enabling epoll since more than one url provided!\n");
    use_epoll = 1;
  }

  if (use_epoll) {
    efd = epoll_create1(0);
    if (efd < 0) {
      perror("epoll_create1");
      exit(1);
    }

    /* Create reporting timer */
    if (interval) {
      tfd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
      if (tfd < 0) {
        perror("timerfd_create");
        exit(1);
      }
      timer_event.data.fd = tfd;
      timer_event.events  = EPOLLIN | EPOLLET | EPOLLPRI;

      if (epoll_ctl(efd, EPOLL_CTL_ADD, tfd, &timer_event) < 0) {
        perror("epoll_ctl(timer_event)");
        exit(1);
      }
    }
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
    } else if (verbose > 2) {
      printf(" Addr#%d opened fd = %d\n", i, addrs[i].fd);
    }

    assert(addrs[i].fd < (int)(sizeof(addrs_idx)/sizeof(addrs_idx[0])));
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
    if (addrs[i].iface_name[0] == '\0') {
      addrs[i].iface = INADDR_ANY;
      if (verbose > 2)
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
      if (verbose > 2)
        printf("Executed: '%s' ->\n  %s\n", buf, obuf);
      if ((sscanf(obuf, "multicast %16s via %16s dev %16s src %16s ",
                  mc, via, addrs[i].iface_name, src) != 4) &&
          (sscanf(obuf, "multicast %16s dev %16s src %16s ",
                  mc, addrs[i].iface_name, src) != 3)) {
        fprintf(stderr, "Couldn't parse output of 'ip route get: %s'\n", obuf);
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
      // Note that mcast listening sockets must bind to the INADDR_ANY address
      // or else no packets will be directed to this socket (kernel bug/feature?):
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
      snprintf(iface, sizeof(iface), "%s", inet_ntoa(*((struct in_addr*)&addrs[i].iface)));
      snprintf(mcast, sizeof(mcast), "%s", inet_ntoa(*((struct in_addr*)&addrs[i].mcast_addr)));
      snprintf(src,   sizeof(src),   "%s", inet_ntoa(*((struct in_addr*)&addrs[i].src_addr)));

      printf("#%02d Join %smcast %s%s%s on iface %s:%d %s\n", addrs[i].id,
        addrs[i].src_addr != INADDR_NONE ? "src-spec " : "",
        addrs[i].src_addr != INADDR_NONE ? src : "",
        addrs[i].src_addr ? "@" : "",
        mcast, iface, addrs[i].port,
        addrs[i].title);
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

  /* Set up reporting timeout */

  if (tfd > 0) {
    struct itimerspec timeout;
    long rem = 5000000 - ((tv.tv_sec % 5) * 1000000 + tv.tv_usec);
    long next_time  = now_time + rem;
    memset(&timeout, 0, sizeof(timeout));
    timeout.it_value.tv_sec     = next_time / 1000000;
    timeout.it_value.tv_nsec    = next_time % 1000000;
    timeout.it_interval.tv_sec  = interval;
    timeout.it_interval.tv_nsec = 0;

    struct itimerspec oldt;
    if (tfd >= 0 && timerfd_settime(tfd, TFD_TIMER_ABSTIME, &timeout, &oldt) < 0) {
      perror("timerfd_settime");
      exit(1);
    }

    if (verbose > 2)
      printf("Reporting timer setup in %ld seconds\n",
        timeout.it_value.tv_sec - now_time/1000000);

    for (i=0; i < (int)(sizeof(sorted_addrs) / sizeof(sorted_addrs[0])); i++) {
      int j, sz = addrs_count * sizeof(struct address*);
      sorted_addrs[i] = (address**)malloc(sz);
      for (j=0; j < addrs_count; j++)
        sorted_addrs[i][j] = &addrs[j];
    }
  }

  /*----------------------------------------------------------------------
   * Main data loop
   *--------------------------------------------------------------------*/

  srand(time(NULL));
  setjmp(jbuf);

  while (!terminate) {
    char databuf[16*1024];
    int events_count, n = -1, i;

    if (use_epoll) {
      if (verbose > 4) printf("  Calling epoll(%d)...\n", efd);
      events_count = epoll_wait(efd, events, sizeof(events)/sizeof(events[0]), -1);
      if (verbose > 4) printf("  epoll() -> %d\n", events_count);

      if (events_count < 0) {
        if (errno == EINTR)
          continue;
        perror("epoll_wait");
        exit(1);
      }
      n = 0;
    } else {
      int fd = addrs[0].fd;
      if (verbose > 4) printf("  Calling read(%d, size=%ld)...\n", fd, sizeof(databuf));
      n = read(fd, databuf, sizeof(databuf));
      if (verbose > 4) printf("  Got %d bytes\n", n);
      events_count = 1;
      events[0].data.fd = fd;
      events[0].events  = EPOLLIN;
    }

    for (i=0; i < events_count; ++i) {
      if (events[i].data.fd == tfd) {
        // Reporting timeout
        uint64_t exp;
        while ((n = read(tfd, &exp, sizeof(exp))) > 0 || errno == EINTR);
        if (errno != EAGAIN) {
          perror("read(timerfd-descriptor)");
          exit(1);
        }
        print_report();
        continue;
      }

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
    printf("%-30s| %6.1f KB/s %6d pkts/s| %9ld %sB %9ld %spkts | OutOfSeq %ld | Lost: %ld\n",
      label ? label : "TOTAL",
      tot_bytes / 1024 / sec, (int)(tot_pkts / sec),
      (long)scale(tot_bytes, 1024), scale_suffix(tot_bytes, 1024),
      (long)scale(tot_pkts,  1000), scale_suffix(tot_pkts,  1000),
      tot_ooo_count, tot_gap_count);
  }

  for (i=0; i < (int)(sizeof(sorted_addrs) / sizeof(sorted_addrs[0])); i++)
    if (sorted_addrs[i])
      free(sorted_addrs[i]);

  if (efd != -1) close(efd);
  if (wfd != -1) close(wfd);

  return tot_pkts ? 0 : 1;
}

static int intcmpa(long a, long b) { return a < b ? -1 : a > b; }
static int intcmpd(long a, long b) { return a > b ? -1 : a < b; }

static int crep_ooo_count(const struct address* a) {
  return a->ooo_count - a->last_crep_ooo_count;
}
static int crep_gap_count(const struct address* a) {
  return a->gap_count - a->last_crep_gap_count;
}
static int crep_pkt_count(const struct address* a) {
  return a->pkt_count - a->last_crep_pkt_count;
}

int sort_by_bytes(const void* a, const void* b) {
  struct address* lhs = *(struct address**)a;
  struct address* rhs = *(struct address**)b;
  int n = intcmpd(lhs->bytes_cnt, rhs->bytes_cnt);
  return n ? n : intcmpa(lhs->port, rhs->port);
}

int sort_by_packets(const void* a, const void* b) {
  struct address* lhs = *(struct address**)a;
  struct address* rhs = *(struct address**)b;
  int n = intcmpd(lhs->pkt_count, rhs->pkt_count);
  return n ? n : intcmpa(lhs->port, rhs->port);
}

int sort_by_ooo(const void* a, const void* b) {
  struct address* lhs = *(struct address**)a;
  struct address* rhs = *(struct address**)b;
  int x = crep_ooo_count(lhs);
  int y = crep_ooo_count(rhs);
  int n = intcmpd(x, y);
  return n ? n : intcmpa(lhs->port, rhs->port);
}

int sort_by_gaps(const void* a, const void* b) {
  struct address* lhs = *(struct address**)a;
  struct address* rhs = *(struct address**)b;
  int x = crep_gap_count(lhs);
  int y = crep_gap_count(rhs);
  int n = intcmpd(x, y);
  return n ? n : intcmpa(lhs->port, rhs->port);
}

void report_socket_stats() {
  typedef int (*compar_fun)(const void*, const void*);
  static const compar_fun sort_funs[] = {
    &sort_by_bytes, &sort_by_packets, &sort_by_ooo, &sort_by_gaps
  };
  static const char SEP[] = "================================"
                            "================================"
                            "================================"
                            "================================"
                            "================================"
                            "================================"
                            "================================"
                            "================================"
                            "================================"
                            "================================";
  //static const char BAR[] = "********************************";
  static const int seqno_width = 9;
  const        int pad_title   = max_title_width - 5;

  int i, max_ooo_count = 0, max_pkt_count = 0, max_bytes = 0, max_gap_count = 0;
  int n = addrs_count > max_channel_report_lines ? max_channel_report_lines : addrs_count;

  for(i = 0; i < addrs_count; i++) {
    struct address* p = addrs + i;
    if (p->bytes_cnt > max_bytes    ) max_bytes     = p->bytes_cnt;
    if (p->pkt_count > max_pkt_count) max_pkt_count = p->pkt_count;
    if (p->ooo_count > max_ooo_count) max_ooo_count = p->ooo_count;
    if (p->gap_count > max_gap_count) max_gap_count = p->gap_count;
  }

  for (i=0; i < (int)(sizeof(sort_funs)/sizeof(sort_funs[0])); i++)
    qsort(sorted_addrs[i], addrs_count, sizeof(struct address*), sort_funs[i]);

  printf("#C|%*.*sTitle|==MBytes|%*.*sLastSeqno|%*.*sTitle|==Packets|%*.*sLastSeqno|\n",
    pad_title, pad_title, SEP, seqno_width-9, seqno_width-9, SEP,
    pad_title, pad_title, SEP, seqno_width-9, seqno_width-9, SEP);

  for(i=0; i < n; i++) {
    struct address* pbytes = sorted_addrs[0][i];
    struct address* ppkts  = sorted_addrs[1][i];
    if (!pbytes->bytes_cnt && !ppkts->pkt_count)
      break;
    //int gbytes = max_bytes     ? (int)(seqno_width * pbytes->bytes_cnt / max_bytes) : 0;
    //int gpkts  = max_pkt_count ? (int)(seqno_width * ppkts->pkt_count / max_pkt_count) : 0;

    printf("#C|%*s|%8.1f|%*ld|%*s|%9d|%*ld|\n",
      max_title_width, pbytes->title, (double)pbytes->bytes_cnt/MEGABYTE,
      seqno_width, pbytes->last_seqno,
      max_title_width, ppkts ->title, ppkts->pkt_count,
      seqno_width, ppkts->last_seqno);
  }

  // Has any non-zero data?
  if (crep_ooo_count(sorted_addrs[2][0]) || crep_gap_count(sorted_addrs[3][0]))
    printf("#c|%*.*sTitle|====Gaps|%*.*sLastSeqno|%*.*sTitle|==OutOrdr|%*.*sLastSeqno|\n",
      pad_title, pad_title, SEP, seqno_width-9, seqno_width-9,  SEP,
      pad_title, pad_title, SEP, seqno_width-9, seqno_width-9, SEP);

  for(i=0; i < n; i++) {
    struct address* pooo   = sorted_addrs[2][i];
    struct address* pgaps  = sorted_addrs[3][i];
    int ooo_count = crep_ooo_count(pooo);
    int gap_count = crep_gap_count(pgaps);
    if (ooo_count || gap_count) {
      //int ggaps = max_gap_count ? (int)(seqno_width * gap_count / max_gap_count) : 0;
      //int gooos = max_ooo_count ? (int)(seqno_width * ooo_count / max_ooo_count) : 0;

      printf("#c|%*s|%8d|%*ld|%*s|%9d|%*ld|\n",
        max_title_width,    gap_count ? pgaps ->title     : "", gap_count,
        seqno_width,        gap_count ? pgaps->last_seqno : 0,
        max_title_width,    ooo_count ? pooo  ->title     : "", ooo_count,
        seqno_width,        ooo_count ? pooo->last_seqno  : 0);
    }
  }

  int width = max_title_width+1+8+seqno_width+1+max_title_width+1+9+seqno_width+2;
  int nodata_count = 0;

  for(i=0; i < addrs_count; i++)
    if (!crep_pkt_count(&addrs[i]) && addrs[i].last_crep_pkt_changed)
      nodata_count++;

  if (nodata_count) {
    printf("#E|EmptyChanged%*.*s|\n", width-12, width-12, SEP);

    int half = addrs_count / 2 + addrs_count % 2;

    for(i=0; i < half; i += 2) {
      int j, e, t = 0;
      for (j=i, e=addrs_count; j < e; j += half) {
        struct address* a = &addrs[j];
        int          pkts = crep_pkt_count(a);
        if (!pkts && a->last_crep_pkt_changed) {
          if (!t) printf("#e|");
          printf("   [%02d] %-*s (%d)", a->id, max_title_width, a->title, pkts);
          t++;
        }
      }
      if (t) printf("\n");
    }
  }

  for(i=0; i < addrs_count; i++) {
    struct address* a = &addrs[i];
    a->last_crep_pkt_changed = crep_pkt_count(a) > 0;
    a->last_crep_ooo_count = a->ooo_count;
    a->last_crep_gap_count = a->gap_count;
    a->last_crep_pkt_count = a->pkt_count;
  }

  printf("#C|%*.*s|\n", width, width, SEP);
}

void print_report() {
  struct timeval tv;
  int i;

  gettimeofday(&tv, NULL);

  if (verbose > 3)
    printf("%06ld Reporting event\n", tv.tv_sec % 86400);

  if (quiet || !(interval && verbose))
    return;

  int output = output_lines_count++;

  if (output >= next_sock_report_lines) {
    report_socket_stats();
    next_sock_report_lines = output_lines_count + sock_interval;
  }

  if (output >= next_legend_count) {
    printf(
      "#S|Sok:%4d| KBytes/s|Pkts/s|OutOfO|SqGap|Es|Gs|Os|TOT"
      "|  MBytes| KPakets|OutOfOrd| TotGaps|Lat N| Avg|Mn|  Max|\n",
      addrs_count);
    next_legend_count = output_lines_count + 50;
  }

  // We skip first reporting period as it may be skewed due
  // to slow subscription startup
  if (output) {
    double sec = (double)(now_time - last_time)/1000000;
    struct tm* tm  = localtime(&tv.tv_sec);
    double avg_lat = pkt_time_count ? (double)sum_pkt_time / pkt_time_count : 0.0;
    int socks_with_gaps = 0, socks_with_ooo = 0, socks_with_nodata = 0;

    for(i = 0; i < addrs_count; i++) {
      struct address* addr = addrs + i;
      if (addr->ooo_count - addr->last_srep_ooo_count)    socks_with_ooo++;
      if (addr->gap_count - addr->last_srep_gap_count)    socks_with_gaps++;
      if (!(addr->pkt_count - addr->last_srep_pkt_count)) socks_with_nodata++;

      addr->last_srep_ooo_count = addr->ooo_count;
      addr->last_srep_gap_count = addr->gap_count;
      addr->last_srep_pkt_count = addr->pkt_count;
    }

    if (sec == 0.0) sec = 1.0;

    printf("II|%02d:%02d:%02d|%9.1f|%6d|%6ld|%5ld|%2d|%2d|%2d|TOT|"
           "%8.1f|%8ld|%8ld|%8ld|%5ld|%4.1f|%2ld|%5ld|\n",
        tm->tm_hour, tm->tm_min, tm->tm_sec,
        (double)bytes / 1024 / sec, (int)(pkts / sec),
        ooo_count, gap_count,
        socks_with_nodata, socks_with_gaps, socks_with_ooo,
        (double)tot_bytes / MEGABYTE, (long)(tot_pkts / 1000),
        tot_ooo_count,  tot_gap_count,
        pkt_time_count, avg_lat,
        pkt_time_count ? min_pkt_time : 0,
        max_pkt_time);
  }

  min_pkt_time  = LONG_MAX;
  max_pkt_time  = 0;
  sum_pkt_time  = 0;
  pkt_time_count= 0;
  last_pkts     = pkts;
  bytes = pkts  = 0;
  ooo_count     = 0;
  gap_count     = 0;
  last_time     = now_time;

  fflush(stdout);
}

void process_packet(struct address* addr, const char* buf, int n) {
  struct timeval tv;
  long seqno;

  gettimeofday(&tv, NULL);
  now_time = tv.tv_sec * 1000000 + tv.tv_usec;

  /* Get timestamp of the packet */
  if ((last_pkts < 1000 && pkts < 1000) || (rand() % 100) < 10) {
    ioctl(addr->fd, SIOCGSTAMP, &tv);
    pkt_time = now_time - (tv.tv_sec * 1000000 + tv.tv_usec);
    sum_pkt_time += pkt_time;
    if (pkt_time < min_pkt_time) min_pkt_time = pkt_time;
    if (pkt_time > max_pkt_time) max_pkt_time = pkt_time;
    pkt_time_count++;
  }

  addr->last_data_time = now_time;
  addr->bytes_cnt += n;
  addr->pkt_count++;

  tot_bytes += n;
  tot_pkts++;
  bytes += n;
  pkts++;

  int seq_reset;
  seqno = get_seqno(addr, buf, n, addr->last_seqno, &seq_reset);

  if (display_packets) {
    fprintf(stderr, "  %02d (fmt=%c) seqno=%ld (pkt size=%d):\n   {",
      addr->id, addr->data_format, seqno, n);
    int i = 0, e = n > display_packets ? display_packets : n;
    if (display_packets_hex)
      for (; i < e; i++) {
        fprintf(stderr, "%s0x%02x", i ? "," : "", (uint8_t)buf[i]);
        if (((i+1) % 16) == 0) fprintf(stderr, "\n   ");
      }
    else {
      const char* p = buf, *end = p + e;
      for (; p != end; p++, i++) {
        fprintf(stderr, "%c", *p > ' ' && *p != char(255) ? *p : '.');
        if (((i+1) % 80) == 0) fprintf(stderr, "\n   ");
      }
    }
    fprintf(stderr, "};\n");
  }

  if (wfd != -1) {
    if (write(wfd, buf, n) < 0) {
      fprintf(stderr, "Error writing to the output file %s: %s\n",
        write_file, strerror(errno));
      exit(1);
    }
  }

  if (seqno) {
    if (addr->last_seqno) {
      int diff = seqno - addr->last_seqno;
      if (!seq_reset) {
        if (diff < 0) {
          if (verbose > 1)
            printf("  %02d Out of order seqno (last=%ld, now=%ld): %d (%s)\n",
              addr->id, addr->last_seqno, seqno, diff, addr->title);
          addr->last_ooo_time = now_time;
          addr->ooo_count++;
          tot_ooo_count++;  /* out of order */
          ooo_count++;
        } else if (diff > 1) {
          addr->last_gap_time = now_time;
          addr->gap_count++;
          tot_gap_count++;
          gap_count++;
          if (verbose > 1)
            printf("  %02d Gap detected in seqno (last=%ld, now=%ld): %d (%s)\n",
              addr->id, addr->last_seqno, seqno, diff, addr->title);
        }

      }
    }
    if (verbose > 3)
      printf("%02d -> %ld (last_seqno=%ld)\n", addr->id, seqno, addr->last_seqno);

    addr->last_seqno = seqno;
  }

  if (tot_pkts >= max_pkts)
    terminate = 1;

  if (verbose > 2)
    printf("Received %6d bytes, %ld packets (%s)\n", n, tot_pkts, addr->title);
}

/*
static int u32_to_size (uint32_t n)
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
*/

static int find_stopbit_byte(const char* buff, const char* end) {
    const uint64_t s_mask = 0x8080808080808080ul;
    const uint64_t*     p = (const uint64_t*)buff;

    uint64_t unmasked = *p & s_mask;
    int pos;

    if (likely(unmasked)) {
        pos = __builtin_ffsl(unmasked) >> 3;
    } else {
        // In case the stop bit is not found in 64 bits,
        // we need to check next bytes
        unmasked = *(++p) & s_mask;
        pos = 8 + (__builtin_ffsl(unmasked) >> 3);
    }

    if (buff + pos < end)
      return pos;

    return 0;
}


int decode_uint_loop(const char** buff, const char* end, uint64_t* val) {
  const char* p = *buff;
  int e, i = 0;
  uint64_t n = 0;

  int len = find_stopbit_byte(p, end);

  //printf("find_stopbit_byte(%02x) -> %d\n", (uint8_t)*p, len);

  for (p += len-1, e = len*7; i < e; i += 7, --p) {
    uint64_t m = (uint64_t)(*p & 0x7F) << i;
    n |= m;
    //printf(" %2d| 0x%02x, m=%lu, n=%lu\n", i / 7, (uint8_t)*p, m, n);
  }

  *val  = n;
  *buff += len;
  return len;
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

//-------------------------------------------------------------------------//
// Extracting "PMap" and TemplateID:                                       //
//-------------------------------------------------------------------------//
// NB: "PMap" bits are stored in the resulting "uint64" in the order of incr-
// easing significance, from 0 to 62 (bit 63 is unused).
// Also note: "tid" may be NULL, in which case we get only the "PMap" (useful
// for processing Sequences):
//
uint32_t decode_forts_seqno(const char* buff, int n, long last_seqno, int* seq_reset) {
    const char* q = buff;
    uint64_t  tid = 120, seq = 0, pmap;

    while (tid == 120) { // reset
      decode_uint_loop(&q, q+5, &pmap);
      decode_uint_loop(&q, q+5, &tid);
    }

    //printf("PMAP=%x, TID=%d, *p=0x%02x\n", pmap, tid, (uint8_t)*q);
    decode_uint_loop(&q, q+5, &seq);

    // If sequence reset, parse new seqno
    if (tid == 49) {
      *seq_reset = 1;
      decode_uint_loop(&q, q+10, &tid); // SendingTime
      decode_uint_loop(&q, q+5,  &seq); // NewSeqNo
    }

    return seq;
}

void test_forts_decode() {
  const uint8_t buffers0[] = 
    {0xc0,0xf8,0xe0,0xca,0x6f,0x41,0xd8,0x23,0x63,0x2d,0x12,0x54,0x66,0x6d,0xf4,0x87,0x98
    ,0xb1,0x30,0x2d,0x44,0xc7,0x22,0xec,0x0f,0x0a,0xc8,0x95,0x82,0x80,0xff,0x00,0x62
    ,0xa7,0x89,0x80,0x00,0x52,0x11,0x55,0xeb,0x80,0x80,0x80,0x80,0x80,0xc0,0x81,0xb1
    ,0x81,0x0f,0x0a,0xc9,0x83,0x80,0xff,0x00,0x62,0xa8,0x00,0xf1,0x80,0x80,0x80,0x80
    ,0x80,0x80,0x80,0x80,0xb1,0x81,0x0f,0x0a,0xca,0x85,0x80,0xff,0x00,0x62,0xaa,0x00
    ,0xe5,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0xb1,0x74,0x03,0x32,0x80,0x15,0x4f
    ,0xec,0x83,0x80,0x82,0x00,0x68,0x9f,0x89,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80
    ,0xb1,0x81,0x15,0x4f,0xed,0x84,0x80,0x82,0x00,0x68,0xa0,0x8d,0x80,0x81,0x80,0x80
    ,0x80,0x80,0x80,0x80,0xb1,0x81,0x15,0x4f,0xee,0x85,0x80,0x82,0x00,0x68,0xa1,0x88
    ,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0xb1,0x0f,0x0e,0x52,0x81,0x1c,0x21,0xc4
    ,0x82,0x80,0x81,0x00,0x4c,0x9b,0x8c,0x80,0x80,0x80,0x80,0x80,0x80,0x80};

  const uint8_t buffers1[] =
    {0xc0,0xf8,0xe0,0xca,0x6f,0x41,0xd9,0x23,0x63,0x2d,0x12,0x54,0x66,0x6e,0x82,0x81,0xd8
    ,0x81,0xb1,0x33,0x3f,0x48,0xc7,0x22,0xec,0x1c,0x21,0xc5,0x95,0x82,0x80,0x81,0x00
    ,0x4c,0x9b,0x8b,0x80,0x00,0x52,0x11,0x55,0xfd,0x80,0x80,0x80,0x80,0x80};

  const uint8_t buffers2[] =
    {0xc0,0xf8,0xe0,0xca,0x6f,0x41,0xda,0x23,0x63,0x2d,0x12,0x54,0x66,0x6e,0x90,0x85,0xd8
    ,0x82,0xb1,0x33,0x3f,0x48,0xc7,0x22,0xec,0x1c,0x21,0xc6,0x95,0x82,0x80,0x81,0x00
    ,0x4c,0x9b,0x81,0x80,0x00,0x52,0x11,0x55,0xfd,0x80,0x80,0x80,0x80,0x80,0xc0,0x80
    ,0xb1,0x81,0x1c,0x21,0xc7,0x95,0x80,0x81,0x00,0x4c,0xaf,0x04,0xaa,0x80,0x7f,0x0b
    ,0x6d,0xb6,0x80,0x80,0x80,0x80,0x80,0x80,0xb0,0x7c,0x6d,0x74,0x80,0x1e,0x6b,0xef
    ,0x82,0x80,0x82,0x00,0x73,0xf1,0x87,0x80,0x00,0x74,0x12,0xda,0x80,0x80,0x80,0x80
    ,0x80,0x80,0xb0,0x81,0x1e,0x6b,0xf0,0x83,0x80,0x82,0x00,0x73,0xf0,0x86,0x80,0xfc
    ,0x80,0x80,0x80,0x80,0x80,0xc0,0x81,0xb0,0x81,0x1e,0x6b,0xf1,0x84,0x80,0x82,0x00
    ,0x73,0xef,0x89,0x80,0x84,0x80,0x80,0x80,0x80,0x80};

  const uint8_t buffers3[] =
    {0xc0,0xf8,0xe0,0xca,0x6f,0x41,0xdb,0x23,0x63,0x2d,0x12,0x54,0x66,0x6e,0xd9,0x82,0xd8
    ,0x82,0xb0,0x30,0x2d,0x3c,0xc7,0x22,0xec,0x1e,0x6b,0xf2,0x95,0x83,0x80,0x82,0x00
    ,0x73,0xf0,0x81,0x80,0x00,0x52,0x11,0x56,0xdb,0x80,0x80,0x80,0x80,0x80,0xc0,0x80
    ,0xb0,0x81,0x1e,0x6b,0xf3,0x95,0x80,0x82,0x00,0x71,0xe5,0x82,0x80,0x72,0x7b,0x1a
    ,0xde,0x80,0x80,0x80,0x80,0x80};

  const uint8_t* buffers[] = {buffers0, buffers1, buffers2, buffers3};

  int i;
  for (i=0; i < (int)(sizeof(buffers)/sizeof(buffers[0])); i++) {
    const char* q = (const char*)buffers[i];
    long last = 0;
    int seq_reset;
    uint32_t res = decode_forts_seqno(q, 40, last, &seq_reset);
    printf("#%d res=%d\n", i, res);
  }
}
