/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   checksum.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bcaumont <bcaumont@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/05 13:20:14 by bcaumont          #+#    #+#             */
/*   Updated: 2026/11/05 15:12:23 by bcaumont         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h"

uint16_t calculate_checksum(const void *data, size_t length)
{
    const unsigned char *bytes;
    uint32_t             sum;
    uint16_t             word;

    bytes = (const unsigned char *)data;
    sum = 0;
    while (length > 1)
    {
        memcpy(&word, bytes, sizeof(word));
        sum += word;
        bytes += 2;
        length -= 2;
    }
    if (length == 1)
        sum += *bytes;
    while (sum >> 16)
        sum = (sum & 0xffffU) + (sum >> 16);
    return ((uint16_t)~sum);
}

