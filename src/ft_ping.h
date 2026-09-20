#ifndef FT_PING_H
#define FT_PING_H

#include <netinet/in.h>

int parse(const char *, struct sockaddr_storage *);
unsigned short compute_checksum(unsigned short *, int);

#endif
