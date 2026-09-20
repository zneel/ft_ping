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

    int ret = connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (ret == -1)
    {
        perror("Failed to connect to destination");
        close(sockfd);
        return EXIT_FAILURE;
    }

    signal(SIGINT, request_stop);

    int id = getpid();
    int sent = 0;
    int received = 0;
    double rtt_min = INFINITY, rtt_max = 0.0, rtt_sum = 0.0;
    struct pollfd s_poll = {.fd = sockfd, .events = POLLIN};
    while (g_running)
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        char sbuff[64] = {0};
        struct icmp *packet = (struct icmp *)sbuff;
        packet->icmp_type = ICMP_ECHO;
        packet->icmp_code = 0;
        packet->icmp_cksum = 0;
        packet->icmp_id = htons(id);
        packet->icmp_seq = htons(sent);
        memcpy(sbuff + ICMP_MINLEN, &tv, sizeof(tv));

        packet->icmp_cksum = compute_checksum((unsigned short *)sbuff, sizeof(sbuff));
        ssize_t bytes_sent = send(sockfd, sbuff, sizeof(sbuff), 0);
        if (bytes_sent == -1)
        {
            perror("Failed to send ICMP packet");
            close(sockfd);
            return EXIT_FAILURE;
        }
        sent++;

        struct timeval deadline;
        gettimeofday(&deadline, NULL);
        deadline.tv_sec += 1;
        for (;;)
        {
            int remaining = ms_until(&deadline);
            if (remaining <= 0)
                break;

            int p = poll(&s_poll, 1, remaining);
            if (p == 0)
                break;
            if (p == -1)
            {
                if (errno == EINTR) // interupted
                    break;
                perror("Poll error");
                close(sockfd);
                return EXIT_FAILURE;
            }
            if (!(s_poll.revents & POLLIN))
            {
                printf("Unexpected event occurred: %d\n", s_poll.revents);
                close(sockfd);
                return EXIT_FAILURE;
            }
            char buff[1024];
            ssize_t bytes_received = recv(sockfd, buff, sizeof(buff), 0);
            if (bytes_received == -1)
            {
                perror("Failed to receive ICMP packet");
                close(sockfd);
                return EXIT_FAILURE;
            }
            received++;

            struct ip *ip = (struct ip *)buff;
            ssize_t ip_header_len = ip->ip_hl * 4;
            struct icmp *icmp_reply = (struct icmp *)(buff + ip_header_len);
            struct timeval *sent_time = (struct timeval *)(buff + ip_header_len + ICMP_MINLEN);
            struct timeval now;
            gettimeofday(&now, NULL);
            double elapsed_time = (now.tv_sec - sent_time->tv_sec) * 1000.0 + (now.tv_usec - sent_time->tv_usec) / 1000.0;
            rtt_min = (rtt_min == INFINITY) ? elapsed_time : fmin(rtt_min, elapsed_time);
            rtt_max = fmax(rtt_max, elapsed_time);
            rtt_sum += elapsed_time;
            printf("%zd bytes received from %s: icmp_seq=%d ttl=%d time=%.3f ms\n",
                   bytes_received - ip_header_len,
                   argv[1],
                   ntohs(icmp_reply->icmp_seq)+1,
                   ip->ip_ttl,
                   elapsed_time);
        }
    }

    printf("\n--- %s ping statistics ---\n", argv[1]);
    printf("%d packets transmitted, %d received, %.1f%% packet loss\n",
           sent,
           received,
           sent ? (double)(sent - received) / sent * 100 : 0.0);
    printf("rtt min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms\n",
           rtt_min, rtt_sum / received, rtt_max, sqrt((rtt_sum / received) * (rtt_sum / received) - (rtt_min * rtt_min)));

    close(sockfd);
    return 0;
}
