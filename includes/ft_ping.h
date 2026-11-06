/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_ping.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bcaumont <bcaumont@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/05 15:13:20 by bcaumont          #+#    #+#             */
/*   Updated: 2026/11/05 15:13:22 by bcaumont         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_PING_H
# define FT_PING_H

# include <arpa/inet.h>
# include <errno.h>
# include <limits.h>
# include <netdb.h>
# include <netinet/in.h>
# include <netinet/ip.h>
# include <netinet/ip_icmp.h>
# include <signal.h>
# include <stdint.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <sys/select.h>
# include <sys/socket.h>
# include <time.h>
# include <unistd.h>

# define DEFAULT_PAYLOAD_SIZE 56
# define MAX_PAYLOAD_SIZE 65499
# define MAX_PACKET_SIZE (sizeof(struct icmphdr) + MAX_PAYLOAD_SIZE)
# define PATTERN_MAX_SIZE 16

# define TS_TSONLY 0
# define TS_TSADDR 1

typedef struct s_ping
{
    char                *target;
    char                *pattern;
    int                 sockfd;
    int                 verbose;
    int                 help;
    int                 flood;
    int                 numeric;
    int                 ignore_route;
    int                 ip_timestamp;
    int                 timestamp_mode;
    int                 tos;
    int                 ttl;
    int                 packet_size;
    int                 timeout;
    int                 deadline;
    int                 preload;
    int                 count;
    uint16_t            identifier;
    uint16_t            sequence;
    unsigned long       packets_sent;
    unsigned long       packets_received;
    unsigned long       errors_received;
    double              rtt_min;
    double              rtt_max;
    double              rtt_sum;
    struct sockaddr_in  dest_addr;
    struct timespec     *sent_times;
}   t_ping;

typedef struct s_reply
{
    size_t          bytes;
    int             ttl;
    int             icmp_type;
    int             icmp_code;
    uint16_t        sequence;
    double          rtt_ms;
    struct in_addr  source;
}   t_reply;

int         parse_args(int argc, char **argv, t_ping *ping);
void        print_usage(void);
int         resolve_target(t_ping *ping);
int         create_socket(t_ping *ping);
uint16_t    calculate_checksum(const void *data, size_t length);
int         build_icmp_packet(t_ping *ping, unsigned char *packet,
                size_t *packet_size, const struct timespec *sent_at);
int         send_packet(t_ping *ping, const unsigned char *packet,
                size_t packet_size);
int         receive_icmp_reply(t_ping *ping, t_reply *reply,
                const struct timespec *sent_at, double timeout_seconds);
int         ping_loop(t_ping *ping);

#endif
