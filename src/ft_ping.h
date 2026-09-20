#ifndef FT_PING_H
#define FT_PING_H
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <poll.h>
#include <time.h>
#include <netdb.h>
#include <errno.h>
#include <sys/time.h>
#include <stdarg.h>
#include <math.h>

int parse(const char *, struct sockaddr_storage *);
unsigned short compute_checksum(unsigned short *, int);
int ms_until(struct timeval *);

#endif
