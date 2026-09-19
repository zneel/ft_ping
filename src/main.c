#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <poll.h>
#include <time.h>

int is_valid_ip_or_hostname(const char *input)
{
    struct sockaddr_in sa;
    return inet_pton(AF_INET, input, &(sa.sin_addr)) != 0;
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
        fprintf(stderr, "Usage: %s <IP_ADDRESS_OR_HOSTNAME>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int valid_ip = is_valid_ip_or_hostname(argv[1]);
    if (!valid_ip)
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

    struct sockaddr_in dest_addr;
    bzero(&dest_addr, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(argv[1]);


    int ret = connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in));
    if (ret == -1)
    {
        perror("Failed to connect to destination");
        close(sockfd);
        return EXIT_FAILURE;
    }

    int i = 0;
    struct pollfd *s_poll = malloc(sizeof(struct pollfd));
    bzero(s_poll, sizeof(struct pollfd));
    s_poll->fd = sockfd;
    s_poll->events = POLLIN;
    while (1)
    {
        struct pollfd s_poll = {.fd = sockfd, .events = POLLIN};
        int p = poll(&s_poll, 1, 0);
        if (p == -1)
        {
            perror("Poll error");
                close(sockfd);
            return EXIT_FAILURE;
        }
        struct icmp packet = (struct icmp){
            .icmp_type = ICMP_ECHO,
            .icmp_code = 0,
            .icmp_cksum = 0,
            .icmp_id = getpid() & 0xFFFF,
            .icmp_seq = i,
        };
        packet.icmp_cksum = compute_checksum((unsigned short *)&packet, sizeof(packet));
        ssize_t bytes_sent = send(sockfd, &packet, sizeof(packet), 0);
        if (bytes_sent == -1)
        {
            perror("Failed to send ICMP packet");
            close(sockfd);
            return EXIT_FAILURE;
        }
        i++;
        if (p > 0)
        {
            if (s_poll.revents & POLLIN)
            {
                struct icmp *buffer = malloc(sizeof(struct icmp));
                if (buffer == NULL)
                {
                    perror("Failed to allocate memory for buffer");
                    close(sockfd);
                    return EXIT_FAILURE;
                }
                bzero(buffer, sizeof(struct icmp));
                ssize_t bytes_received = recv(sockfd, buffer, sizeof(struct icmp), 0);
                if (bytes_received == -1)
                {
                    perror("Failed to receive ICMP packet");
                    free(buffer);
                    close(sockfd);
                    return EXIT_FAILURE;
                }
                int seq = buffer->icmp_seq;
                printf("%zd bytes received from %s: icmp_seq=%d ttl=%d time=%.3f ms\n",
                       bytes_received,
                       argv[1],
                       seq,
                       ((struct ip *)buffer)->ip_ttl,
                       (double)(clock() - seq) / CLOCKS_PER_SEC * 1000);
            }
            else
            {
                printf("Unexpected event occurred: %d\n", s_poll.revents);
                close(sockfd);
                return EXIT_FAILURE;
            }
        }
        sleep(1);
    }
    printf("--- %s ping statistics ---\n", argv[1]);
    printf("%d packets transmitted, %d received, %.1f%% packet loss\n",
           i + 1,
           i + 1,
           ((double)(i + 1 - (i + 1)) / (i + 1)) * 100);

    close(sockfd);
    return 0;
}
