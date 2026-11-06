/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   resolve_target.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bcaumont <bcaumont@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/05 11:47:13 by bcaumont          #+#    #+#             */
/*   Updated: 2026/11/05 15:15:35 by bcaumont         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h"

int resolve_target(t_ping *ping)
{
    struct addrinfo hints;
    struct addrinfo *result;
    int              status;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;
    hints.ai_protocol = IPPROTO_ICMP;
    status = getaddrinfo(ping->target, NULL, &hints, &result);
    if (status != 0)
    {
        fprintf(stderr, "ft_ping: %s: %s\n", ping->target, gai_strerror(status));
        return (-1);
    }
    if (result == NULL || result->ai_addrlen < sizeof(ping->dest_addr))
    {
        fprintf(stderr, "ft_ping: no IPv4 address found for %s\n", ping->target);
        freeaddrinfo(result);
        return (-1);
    }
    memcpy(&ping->dest_addr, result->ai_addr, sizeof(ping->dest_addr));
    freeaddrinfo(result);
    return (0);
}
