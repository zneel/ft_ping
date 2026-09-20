#include "ft_ping.h"

int ms_until(struct timeval *deadline)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    int remaining = (deadline->tv_sec - now.tv_sec) * 1000 + (deadline->tv_usec - now.tv_usec) / 1000;
    return remaining;
}