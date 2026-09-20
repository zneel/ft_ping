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

#include "ft_ping.h"

static volatile sig_atomic_t g_running = 1;

static void request_stop(int sig)
{
    (void)sig;
    g_running = 0;
}

int parse(const char *input, struct sockaddr_storage *out)
{
    struct addrinfo hints = (struct addrinfo){
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_RAW,
        .ai_protocol = IPPROTO_ICMP,
    };

    struct addrinfo *res;
    int ret = getaddrinfo(input, NULL, &hints, &res);
    if (ret != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        return 0;
    }

    memcpy(out, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    return 1;
}

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

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <IP_ADDRESS>\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct sockaddr_storage dest_addr;
    if (!parse(argv[1], &dest_addr))
    {
        fprintf(stderr, "Invalid IP address or hostname: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd == -1)
    {
        perror("Failed to create socket");
        return EXIT_FAILURE;
    }

    int ret = connect(sockfd, (struct sockaddr *)&dest_addr, dest_addr.ss_len);
    if (ret == -1)
    {
        perror("Failed to connect to destination");
        close(sockfd);
        return EXIT_FAILURE;
    }

    signal(SIGINT, request_stop);

    int id = getpid() & 0xFFFF;
    int sent = 0;
    int received = 0;
    while (g_running)
    {
        struct pollfd s_poll = {.fd = sockfd, .events = POLLIN};
        int p = poll(&s_poll, 1, 0);
        if (p == -1)
        {
            perror("Poll error");
            close(sockfd);
            return EXIT_FAILURE;
        }

        void *packet = malloc(64);
        struct icmp icmp_part = (struct icmp){
            .icmp_type = ICMP_ECHO,
            .icmp_code = 0,
            .icmp_cksum = 0,
            .icmp_id = id,
            .icmp_seq = sent,
        };
        icmp_part.icmp_cksum = compute_checksum((unsigned short *)&icmp_part, sizeof(icmp_part));
        memcpy(packet, &icmp_part, sizeof(icmp_part));
        struct timeval tv;
        gettimeofday(&tv, NULL);
        memcpy((char *)packet + sizeof(icmp_part), &tv, sizeof(tv));
        ssize_t bytes_sent = send(sockfd, packet, sizeof(packet), 0);
        if (bytes_sent == -1)
        {
            perror("Failed to send ICMP packet");
            close(sockfd);
            return EXIT_FAILURE;
        }
        sent++;
        if (p > 0)
        {
            if (!(s_poll.revents & POLLIN))
            {
                printf("Unexpected event occurred: %d\n", s_poll.revents);
                close(sockfd);
                return EXIT_FAILURE;
            }
            struct icmp reply;
            memset(&reply, 0, sizeof(reply));
            ssize_t bytes_received = recv(sockfd, &reply, sizeof(reply), 0);
            if (bytes_received == -1)
            {
                perror("Failed to receive ICMP packet");
                close(sockfd);
                return EXIT_FAILURE;
            }
            received++;
            int seq = reply.icmp_seq;
            struct timeval *sent_time = (struct timeval *)((char *)&reply + sizeof(reply));
            struct timeval now;
            gettimeofday(&now, NULL);
            double rtt = (now.tv_sec - sent_time->tv_sec) * 1000.0 + (now.tv_usec - sent_time->tv_usec) / 1000.0;
            printf("%zd bytes received from %s: icmp_seq=%d ttl=%d time=%.3f ms\n",
                   bytes_received,
                   argv[1],
                   seq,
                   ((struct ip *)&reply)->ip_ttl,
                   rtt);
        }
        sleep(1);
    }

    printf("--- %s ping statistics ---\n", argv[1]);
    printf("%d packets transmitted, %d received, %.1f%% packet loss\n",
           sent,
           received,
           sent ? (double)(sent - received) / sent * 100 : 0.0);

    close(sockfd);
    return 0;
}
