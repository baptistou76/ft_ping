/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ping_loop.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bcaumont <bcaumont@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/05 15:14:27 by bcaumont          #+#    #+#             */
/*   Updated: 2026/11/05 15:14:30 by bcaumont         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h"

static volatile sig_atomic_t g_stop = 0;

static void handle_sigint(int signal_number)
{
    (void)signal_number;
    g_stop = 1;
}

static double elapsed_seconds(const struct timespec *start,
        const struct timespec *end)
{
    return ((end->tv_sec - start->tv_sec)
        + (end->tv_nsec - start->tv_nsec) / 1000000000.0);
}

static double remaining_deadline(const t_ping *ping,
        const struct timespec *program_start)
{
    struct timespec now;

    if (ping->deadline == 0)
        return (-1.0);
    if (clock_gettime(CLOCK_MONOTONIC, &now) == -1)
        return (0.0);
    return ((double)ping->deadline
        - elapsed_seconds(program_start, &now));
}


static void print_statistics(t_ping *ping)
{
    double loss;

    printf("\n--- %s ping statistics ---\n", ping->target);
    if (ping->packets_sent == 0)
    {
        printf("0 packets transmitted, 0 received\n");
        return ;
    }
    loss = 100.0 * (ping->packets_sent - ping->packets_received)
        / ping->packets_sent;
    printf("%lu packets transmitted, %lu received, %.1f%% packet loss\n",
        ping->packets_sent, ping->packets_received, loss);
    if (ping->packets_received > 0)
        printf("rtt min/avg/max = %.3f/%.3f/%.3f ms\n", ping->rtt_min,
            ping->rtt_sum / ping->packets_received, ping->rtt_max);
}

static void print_reply(t_ping *ping, const t_reply *reply)
{
    char address[INET_ADDRSTRLEN];

    if (ping->flood)
    {
        if (reply->icmp_type == ICMP_ECHOREPLY)
            putchar('\b');
        fflush(stdout);
        return ;
    }
    if (inet_ntop(AF_INET, &reply->source, address, sizeof(address)) == NULL)
        strcpy(address, "?");
    if (reply->icmp_type == ICMP_ECHOREPLY)
        printf("%zu bytes from %s: icmp_seq=%u ttl=%d time=%.3f ms\n",
            reply->bytes, address, reply->sequence, reply->ttl, reply->rtt_ms);
    else if (ping->verbose)
    {
        ping->errors_received++;
        if (reply->icmp_type == ICMP_DEST_UNREACH)
            printf("From %s icmp_seq=%u Destination Unreachable (code %d)\n",
                address, reply->sequence, reply->icmp_code);
        else if (reply->icmp_type == ICMP_TIME_EXCEEDED)
            printf("From %s icmp_seq=%u Time to live exceeded\n",
                address, reply->sequence);
        else
            printf("From %s icmp_seq=%u ICMP parameter problem\n",
                address, reply->sequence);
    }
}

static int send_one(t_ping *ping, struct timespec *sent_at)
{
    unsigned char packet[MAX_PACKET_SIZE];
    size_t         packet_size;

    if (clock_gettime(CLOCK_MONOTONIC, sent_at) == -1)
    {
        perror("ft_ping: clock_gettime");
        return (-1);
    }
    ping->sequence++;
    if (build_icmp_packet(ping, packet, &packet_size, sent_at) != 0)
        return (-1);
    if (send_packet(ping, packet, packet_size) != 0)
        return (-1);
    if (ping->sent_times != NULL)
        ping->sent_times[ping->sequence] = *sent_at;
    ping->packets_sent++;
    if (ping->flood)
    {
        putchar('.');
        fflush(stdout);
    }
    return (0);
}

static void account_reply(t_ping *ping, const t_reply *reply)
{
    if (reply->icmp_type != ICMP_ECHOREPLY)
        return ;
    ping->packets_received++;
    ping->rtt_sum += reply->rtt_ms;
    if (ping->packets_received == 1 || reply->rtt_ms < ping->rtt_min)
        ping->rtt_min = reply->rtt_ms;
    if (reply->rtt_ms > ping->rtt_max)
        ping->rtt_max = reply->rtt_ms;
}

static int receive_window(t_ping *ping, const struct timespec *sent_at,
        double duration)
{
    struct timespec start;
    struct timespec now;
    t_reply          reply;
    double           remaining;
    int              result;

    if (clock_gettime(CLOCK_MONOTONIC, &start) == -1)
        return (-1);
    while (!g_stop)
    {
        if (clock_gettime(CLOCK_MONOTONIC, &now) == -1)
            return (-1);
        remaining = duration - elapsed_seconds(&start, &now);
        if (remaining <= 0.0)
            break ;
        result = receive_icmp_reply(ping, &reply, sent_at, remaining);
        if (result < 0)
            return (-1);
        if (result == 0)
            break ;
        account_reply(ping, &reply);
        print_reply(ping, &reply);
    }
    return (0);
}

int ping_loop(t_ping *ping)
{
    struct sigaction action;
    struct timespec  program_start;
    struct timespec  sent_at;
    char             address[INET_ADDRSTRLEN];
    double           window;
    double           deadline_left;
    int              preload_left;
    int              last_packet;

    memset(&action, 0, sizeof(action));
    action.sa_handler = handle_sigint;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGINT, &action, NULL) == -1)
    {
        perror("ft_ping: sigaction");
        return (-1);
    }
    g_stop = 0;
    if (inet_ntop(AF_INET, &ping->dest_addr.sin_addr,
            address, sizeof(address)) == NULL)
        return (-1);
    if (ping->verbose)
        printf("PING %s (%s): %d data bytes, id 0x%04x\n",
            ping->target, address, ping->packet_size, ping->identifier);
    else
        printf("PING %s (%s): %d data bytes\n", ping->target, address,
            ping->packet_size);
    if (clock_gettime(CLOCK_MONOTONIC, &program_start) == -1)
        return (-1);
    preload_left = (ping->flood ? 0 : ping->preload);
    while (!g_stop && (ping->count == 0
            || ping->packets_sent < (unsigned long)ping->count))
    {
        deadline_left = remaining_deadline(ping, &program_start);
        if (ping->deadline > 0 && deadline_left <= 0.0)
            break ;
        if (send_one(ping, &sent_at) != 0)
            break ;
        if (preload_left > 0)
        {
            preload_left--;
            if (preload_left > 0)
                continue ;
        }
        last_packet = (ping->count > 0
                && ping->packets_sent >= (unsigned long)ping->count);
        if (ping->flood)
            window = 0.01;
        else if (last_packet)
            window = (double)ping->timeout;
        else
            window = 1.0;
        deadline_left = remaining_deadline(ping, &program_start);
        if (ping->deadline > 0 && deadline_left < window)
            window = (deadline_left > 0.0 ? deadline_left : 0.0);
        if (receive_window(ping, &sent_at, window) != 0)
            break ;
    }
    if (ping->flood)
        putchar('\n');
    print_statistics(ping);
    return (ping->packets_received > 0 ? 0 : 1);
}
