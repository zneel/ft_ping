#include "ft_ping.h"


unsigned short compute_checksum(unsigned short *buf, int len)
{
    unsigned long sum = 0;
    for (int i = 0; i < len / 2; i++)
    {
        sum += buf[i];
    }
    if (len % 2 == 1)
    {
        sum += ((unsigned char *)buf)[len - 1];
    }
    while (sum >> 16)
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return ~sum;
}