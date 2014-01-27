/* vim: ts=2 sw=2 */
/*
 * This program can be used to test presence of multicast traffic
 * and monitor its incoming rate.
 *
 * Copyright (c) 2014 Serge Aleynikov
 * Created: 2014-01-27
 * License: BSD open source
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

sigjmp_buf jbuf;
int        terminate = 0;

void usage(const char* program) {
  printf("Listen to multicast traffic from a given (source addr) address:port\n\n"
         "Usage: %s [-a Addr] [-m Mcastaddr -p Port [-s SourceAddr]] [-v] [-q]\n"
         "          [-i ReportingIntervalSec] [-d DurationSec] [-b RecvBufSize]\n"
         "          [-l ReportingLabel]\n\n"
         "      -a Addr     - Optional interface address or multicast address\n"
         "                    in the form: udp://SourceIp@MulticastIp:Port\n"
         "                             or: udp://SourceIp@MulticastIp;IfAddr:Port\n"
         "                    If interface address is not provided, it'll be\n"
         "                    determined automatically by a call to\n"
         "                       'ip route get...'\n"
         "      -b Size     - Socket receive buffer size\n"
         "      -i Sec      - Reporting interval (default: 5s)\n"
         "      -d Sec      - Execution time in sec (default: infinity)\n"
         "      -l Label    - Title to include in the output report\n"
         "      -v          - Verbose (use -vv for more detailed output\n"
         "      -q          - Quiet (no output)\n\n"
         "If there is no incoming data, press several Ctrl-C to break\n\n"
         "Return code: = 0  - if the process received at least one packet\n"
         "             > 0  - if no packets were received or there was an error\n\n"
         "Example:\n"
         "  %s -a udp://91.203.253.233@239.195.4.11:26011 -v -i 1 -d 3\n\n",
         program, program);
  exit(1);
}

void sig_handler(int sig) {
  terminate++;
  if (sig == SIGALRM)
    siglongjmp(jbuf, 1);
  else if (terminate > 2) {
    fprintf(stderr, "Aboring...\n");
    exit(1);
  }
}

void main (int argc, char *argv[])
{
  struct sockaddr_in    local_s;
  struct ip_mreq        group;
  struct ip_mreq_source group_s;
  int                   sd;
  int                   datalen;
  char                  databuf[64*1024];
  char                  src[16];
  const char*           addr = NULL;
  const char*           mcast_addr = NULL;
  const char*           src_addr = NULL;
  const char*           label = NULL;
  int                   port = 0;
  int                   verbose = 0;
  int                   bsize = 0;
  struct timeval        tv;
  long                  start_time, next_time, now_time, last_time;
  long                  total_bytes = 0, total_pkts = 0;
  int                   i, bytes = 0, pkts = 0;
  long                  interval = 5;
  int                   quiet = 0;

  signal(SIGINT,  sig_handler);
  signal(SIGALRM, sig_handler);

  if (argc < 3)
    usage(argv[0]);

  for (i=1; i < argc; ++i) {
    if (!strcmp(argv[i], "-a") && i < argc-1) {
      char* s = argv[++i];
      if (strncmp(s, "udp://", 6))
        addr = s;
      else {
        char* p = strchr(s+6, '@');
        s += 6;
        if (p) {
          src_addr = s;
          *p++ = '\0';
          s = p;
        } else {
          addr = NULL;
        }
        mcast_addr = s;
        p = strchr(p, ';');
        if (p) {
          *p++ = '\0';
          addr = mcast_addr;
          s = p;
        }
        p = strchr(s, ':');
        if (!p) {
          fprintf(stderr, "Invalid multicast address:port specified: %s", addr);
          exit(1);
        }
        *p++ = '\0';
        port = atoi(p);
      }
    } else if (!strcmp(argv[i], "-p") && i < argc-1)
      port = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-m") && i < argc-1)
      mcast_addr = argv[++i];
    else if (!strcmp(argv[i], "-s") && i < argc-1)
      src_addr = argv[++i];
    else if (!strcmp(argv[i], "-b") && i < argc-1)
      bsize = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-i") && i < argc-1)
      interval = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-d") && i < argc-1) {
      gettimeofday(&tv, NULL);
      alarm(atoi(argv[++i]));
    } else if (!strncmp(argv[i], "-v", 2))
      verbose = strlen(argv[i])-1;
    else if (!strncmp(argv[i], "-q", 2))
      quiet = 1;
    else if (!strcmp(argv[i], "-l") && i < argc-1)
      label = argv[++i];
    else
      usage(argv[0]);
  }

  if (!mcast_addr || !port)
    usage(argv[0]);
  
  if (!addr) {
    char buf[256], mc[32], via[32], dev[32];
    FILE* file;
    snprintf(buf, sizeof(buf), "ip route get %s\n", mcast_addr);
    file = popen(buf, "r");
    if (!file) {
      perror("ip route get");
      exit(1);
    }
    if (fscanf(file, "multicast %16s via %16s dev %16s src %16s\n",
        mc, via, dev, src) != 4) {
      fprintf(stderr, "Couldn't parse output of 'ip route get'\n");
      exit(2);
    }
    addr = src;
  }

  if (!quiet && verbose) {
    printf("Joining %smulticast %s%s%s on interface %s:%d\n",
      src_addr ? "source-specific " : "",
      src_addr ? src_addr : "",
      src_addr ? "@" : "",
      mcast_addr, addr, port);
  }

  /* ------------------------------------------------------------*/
  /*                                                             */
  /* Receive Multicast Datagram code example.                    */
  /*                                                             */
  /* ------------------------------------------------------------*/
 
  /*
   * Create a datagram socket on which to receive.
   */
  sd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sd < 0) {
    perror("opening datagram socket");
    exit(1);
  }
 
  /*
   * Enable SO_REUSEADDR to allow multiple instances of this
   * application to receive copies of the multicast datagrams.
   */
  {
    int reuse=1;
 
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR,
                   (char *)&reuse, sizeof(reuse)) < 0) {
      perror("setting SO_REUSEADDR");
      close(sd);
      exit(1);
    }
  }
 
  /*
   * Set receive buffer size
   */
  if (bsize && setsockopt(sd, SOL_SOCKET, SO_RCVBUF, &bsize, sizeof(bsize))) {
    perror("setting SO_RCVBUF");
    exit(1);
  }

  /*
   * Bind to the proper port number with the IP address
   * specified as INADDR_ANY.
   */
  memset((char *) &local_s, 0, sizeof(local_s));
  local_s.sin_family = AF_INET;
  local_s.sin_port = htons(port);;
  local_s.sin_addr.s_addr  = INADDR_ANY;
 
  if (bind(sd, (struct sockaddr*)&local_s, sizeof(local_s))) {
    perror("binding datagram socket");
    close(sd);
    exit(1);
  }
 
 
  /*
   * Join the multicast (source?) group on the given interface.
   * Note that this IP_ADD_(SOURCE_)MEMBERSHIP option must be
   * called for each local interface over which the multicast
   * datagrams are to be received.
   */

  if (src_addr) {
    group_s.imr_multiaddr.s_addr = inet_addr(mcast_addr);
    group_s.imr_interface.s_addr = inet_addr(addr);
    group_s.imr_sourceaddr.s_addr = inet_addr(src_addr);
    if (setsockopt(sd, IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP,
                   (char *)&group_s, sizeof(group_s)) < 0) {
      perror("adding source multicast group");
      close(sd);
      exit(1);
    }
  } else {
    group.imr_multiaddr.s_addr = inet_addr(mcast_addr);
    group.imr_interface.s_addr = inet_addr(addr);
    if (setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                   (char *)&group, sizeof(group)) < 0) {
      perror("adding multicast group");
      close(sd);
      exit(1);
    }
  }

  gettimeofday(&tv, NULL);
  start_time = now_time = last_time = tv.tv_sec * 1000000 + tv.tv_usec;
  next_time  = start_time + interval * 1000000;

  /*
   * Read from the socket.
   */

  setjmp(jbuf);
  
  while (!terminate) {
    datalen = sizeof(databuf);
    int n = read(sd, databuf, datalen);
    if (n < 0) {
      perror("reading datagram message");
      close(sd);
      exit(1);
    }

    gettimeofday(&tv, NULL);
    now_time = tv.tv_sec * 1000000 + tv.tv_usec;

    total_bytes += n;
    total_pkts++;
    bytes += n;
    pkts++;

    if (verbose > 1)
      printf("Received %6d bytes, %ld packets\n", n, total_pkts);

    if (!quiet && (interval || verbose) && now_time >= next_time) {
      double sec = (double)(now_time - last_time)/1000000;
      struct tm* tm = localtime(&tv.tv_sec);
      if (sec == 0.0) sec = 1.0;
      printf("%02d:%02d:%02d|%6.1f KB/s %6d pkts/s|Total: %9ld %cB %6ld %spkts\n",
        tm->tm_hour, tm->tm_min, tm->tm_sec, sec,
        (double)bytes / 1024 / sec, (int)(pkts / sec),
        total_bytes > (1024*1024) ? total_bytes/1024/1024 : total_bytes/1024,
        total_bytes > (1024*1024) ? 'M' : 'K',
        total_pkts > 1000000 ? total_pkts/1000000 : total_pkts, 
        total_pkts > 1000000 ? "M" : ""); 
      bytes = pkts = 0;
      last_time = now_time;
      next_time = now_time + interval*1000000;
    }
  } 

  gettimeofday(&tv, NULL);

  if (!quiet) {
    double sec = (double)(tv.tv_sec * 1000000 + tv.tv_usec - start_time)/1000000;
    if (sec == 0.0) sec = 1.0;
    printf("%-30s| %6.1f KB/s %6d pkts/s| %9ld %sB %9ld %spkts\n",
      label ? label : "TOTAL",
      total_bytes / 1024 / sec, (int)(total_pkts / sec),
      total_bytes > (1024*1024)
        ? total_bytes/1024/1024
        : total_bytes > 1024
          ? total_bytes / 1024
          : total_bytes,
      total_bytes > (1024*1024)
        ? "M"
        : total_bytes > 1024
          ? "K" : "",
      total_pkts > 1000000
        ? total_pkts/1000000
        : total_pkts > 1000
          ? total_pkts / 1000
          : total_pkts,
      total_pkts > 1000000
        ? "M"
        : total_pkts > 1000
          ? "K" : ""); 
  }

  exit(total_pkts ? 0 : 1);
}
