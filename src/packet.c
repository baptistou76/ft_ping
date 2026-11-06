/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   packet.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bcaumont <bcaumont@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/05 15:12:56 by bcaumont          #+#    #+#             */
/*   Updated: 2026/11/05 15:12:57 by bcaumont         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h"

static int hex_value(char c)
{
    if (c >= '0' && c <= '9')
        return (c - '0');
    if (c >= 'a' && c <= 'f')
        return (c - 'a' + 10);
    return (c - 'A' + 10);
}

static void fill_payload(t_ping *ping, unsigned char *payload)
{
    size_t pattern_len;
    size_t i;

    if (ping->pattern == NULL)
    {
        memset(payload, 0x61, (size_t)ping->packet_size);
        return ;
    }
    pattern_len = strlen(ping->pattern) / 2;
    i = 0;
    while (i < (size_t)ping->packet_size)
    {
        payload[i] = (unsigned char)((hex_value(ping->pattern[(i % pattern_len) * 2]) << 4)
                | hex_value(ping->pattern[(i % pattern_len) * 2 + 1]));
        i++;
    }
}

int build_icmp_packet(t_ping *ping, unsigned char *packet,
        size_t *packet_size, const struct timespec *sent_at)
{
    struct icmphdr *header;

    *packet_size = sizeof(struct icmphdr) + (size_t)ping->packet_size;
    memset(packet, 0, *packet_size);
    header = (struct icmphdr *)packet;
    header->type = ICMP_ECHO;
    header->code = 0;
    header->un.echo.id = htons(ping->identifier);
    header->un.echo.sequence = htons(ping->sequence);
    fill_payload(ping, packet + sizeof(*header));
    (void)sent_at;
    header->checksum = 0;
    header->checksum = calculate_checksum(packet, *packet_size);
    return (0);
}

int send_packet(t_ping *ping, const unsigned char *packet, size_t packet_size)
{
    ssize_t sent;

    sent = sendto(ping->sockfd, packet, packet_size, 0,
            (struct sockaddr *)&ping->dest_addr, sizeof(ping->dest_addr));
    if (sent == -1)
    {
        perror("ft_ping: sendto");
        return (-1);
    }
    if ((size_t)sent != packet_size)
    {
        fprintf(stderr, "ft_ping: short ICMP write\n");
        return (-1);
    }
    return (0);
}
