/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   receive_packet.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bcaumont <bcaumont@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/05 15:15:06 by bcaumont          #+#    #+#             */
/*   Updated: 2026/11/05 15:15:14 by bcaumont         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h"

static double elapsed_ms(const struct timespec *start,
        const struct timespec *end)
{
    return ((end->tv_sec - start->tv_sec) * 1000.0
        + (end->tv_nsec - start->tv_nsec) / 1000000.0);
}

static int packet_matches(t_ping *ping, unsigned char *buffer,
        ssize_t length, t_reply *reply)
{
    struct iphdr   *ip;
    struct icmphdr *icmp;
    size_t         ip_len;
    unsigned char  *inner;
    struct iphdr   *inner_ip;
    struct icmphdr *inner_icmp;

    if (length < (ssize_t)sizeof(struct iphdr))
        return (0);
    ip = (struct iphdr *)buffer;
    ip_len = (size_t)ip->ihl * 4;
    if (ip->ihl < 5 || length < (ssize_t)(ip_len + sizeof(*icmp)))
        return (0);
    icmp = (struct icmphdr *)(buffer + ip_len);
    reply->icmp_type = icmp->type;
    reply->icmp_code = icmp->code;
    if (icmp->type == ICMP_ECHOREPLY)
    {
        if (ntohs(icmp->un.echo.id) != ping->identifier)
            return (0);
        reply->sequence = ntohs(icmp->un.echo.sequence);
        if (ping->sent_times != NULL
            && ping->sent_times[reply->sequence].tv_sec == 0
            && ping->sent_times[reply->sequence].tv_nsec == 0)
            return (0);
        if (ntohs(ip->tot_len) < ip_len)
            return (0);
        reply->bytes = (size_t)ntohs(ip->tot_len) - ip_len;
        reply->ttl = ip->ttl;
        return (1);
    }
    if (icmp->type == ICMP_DEST_UNREACH || icmp->type == ICMP_TIME_EXCEEDED
        || icmp->type == ICMP_PARAMETERPROB)
    {
        inner = (unsigned char *)icmp + sizeof(*icmp);
        if (length < (ssize_t)(ip_len + sizeof(*icmp) + sizeof(struct iphdr)))
            return (0);
        inner_ip = (struct iphdr *)inner;
        if (inner_ip->ihl < 5)
            return (0);
        if (length < (ssize_t)(ip_len + sizeof(*icmp)
                + (size_t)inner_ip->ihl * 4 + sizeof(*inner_icmp)))
            return (0);
        inner_icmp = (struct icmphdr *)(inner + (size_t)inner_ip->ihl * 4);
        if (inner_icmp->type != ICMP_ECHO
            || ntohs(inner_icmp->un.echo.id) != ping->identifier)
            return (0);
        reply->sequence = ntohs(inner_icmp->un.echo.sequence);
        if (ping->sent_times != NULL
            && ping->sent_times[reply->sequence].tv_sec == 0
            && ping->sent_times[reply->sequence].tv_nsec == 0)
            return (0);
        reply->bytes = 0;
        reply->ttl = ip->ttl;
        return (2);
    }
    return (0);
}

int receive_icmp_reply(t_ping *ping, t_reply *reply,
        const struct timespec *sent_at, double timeout_seconds)
{
    unsigned char      buffer[65536];
    struct sockaddr_in sender;
    struct timespec    now;
    struct timespec    end;
    struct timespec    packet_sent_at;
    struct timeval     timeout;
    fd_set             readfds;
    socklen_t          sender_len;
    ssize_t            received;
    double             remaining;
    int                selected;
    int                matched;

    if (clock_gettime(CLOCK_MONOTONIC, &now) == -1)
        return (-1);
    end = now;
    end.tv_sec += (time_t)timeout_seconds;
    end.tv_nsec += (long)((timeout_seconds - (int)timeout_seconds) * 1000000000.0);
    if (end.tv_nsec >= 1000000000L)
    {
        end.tv_sec++;
        end.tv_nsec -= 1000000000L;
    }
    while (1)
    {
        if (clock_gettime(CLOCK_MONOTONIC, &now) == -1)
            return (-1);
        remaining = (end.tv_sec - now.tv_sec)
            + (end.tv_nsec - now.tv_nsec) / 1000000000.0;
        if (remaining <= 0.0 && timeout_seconds > 0.0)
            return (0);
        if (remaining < 0.0)
            remaining = 0.0;
        timeout.tv_sec = (time_t)remaining;
        timeout.tv_usec = (suseconds_t)((remaining - timeout.tv_sec) * 1000000.0);
        FD_ZERO(&readfds);
        FD_SET(ping->sockfd, &readfds);
        selected = select(ping->sockfd + 1, &readfds, NULL, NULL, &timeout);
        if (selected == -1)
        {
            if (errno == EINTR)
                return (0);
            perror("ft_ping: select");
            return (-1);
        }
        if (selected == 0)
            return (0);
        sender_len = sizeof(sender);
        received = recvfrom(ping->sockfd, buffer, sizeof(buffer), 0,
                (struct sockaddr *)&sender, &sender_len);
        if (received == -1)
        {
            if (errno == EINTR)
                return (0);
            perror("ft_ping: recvfrom");
            return (-1);
        }
        matched = packet_matches(ping, buffer, received, reply);
        if (matched == 0)
            continue ;
        reply->source = sender.sin_addr;
        if (clock_gettime(CLOCK_MONOTONIC, &now) == -1)
            return (-1);
        if (matched == 1 && ping->sent_times != NULL)
        {
            packet_sent_at = ping->sent_times[reply->sequence];
            reply->rtt_ms = elapsed_ms(&packet_sent_at, &now);
        }
        else
            reply->rtt_ms = elapsed_ms(sent_at, &now);
        return (matched);
    }
}
