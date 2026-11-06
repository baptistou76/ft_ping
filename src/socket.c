/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   socket.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bcaumont <bcaumont@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/05 12:06:30 by bcaumont          #+#    #+#             */
/*   Updated: 2026/11/05 15:15:47 by bcaumont         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h"

static int configure_ip_timestamp(int sockfd, int mode)
{
    unsigned char options[40];

    memset(options, IPOPT_NOP, sizeof(options));
    options[0] = IPOPT_TIMESTAMP;
    options[1] = sizeof(options);
    options[2] = 5;
    options[3] = (unsigned char)mode;
    options[4] = 0;
    if (setsockopt(sockfd, IPPROTO_IP, IP_OPTIONS,
            options, sizeof(options)) == -1)
    {
        perror("ft_ping: setsockopt(IP_OPTIONS)");
        return (-1);
    }
    return (0);
}

int create_socket(t_ping *ping)
{
    int one;

    ping->sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (ping->sockfd == -1)
    {
        perror("ft_ping: socket (try running with sudo)");
        return (-1);
    }
    if (setsockopt(ping->sockfd, IPPROTO_IP, IP_TTL,
            &ping->ttl, sizeof(ping->ttl)) == -1)
    {
        perror("ft_ping: setsockopt(IP_TTL)");
        close(ping->sockfd);
        ping->sockfd = -1;
        return (-1);
    }
    if (setsockopt(ping->sockfd, IPPROTO_IP, IP_TOS,
            &ping->tos, sizeof(ping->tos)) == -1)
    {
        perror("ft_ping: setsockopt(IP_TOS)");
        close(ping->sockfd);
        ping->sockfd = -1;
        return (-1);
    }
    if (ping->ignore_route)
    {
        one = 1;
        if (setsockopt(ping->sockfd, SOL_SOCKET, SO_DONTROUTE,
                &one, sizeof(one)) == -1)
        {
            perror("ft_ping: setsockopt(SO_DONTROUTE)");
            close(ping->sockfd);
            ping->sockfd = -1;
            return (-1);
        }
    }
    if (ping->ip_timestamp
        && configure_ip_timestamp(ping->sockfd, ping->timestamp_mode) != 0)
    {
        close(ping->sockfd);
        ping->sockfd = -1;
        return (-1);
    }
    return (0);
}
