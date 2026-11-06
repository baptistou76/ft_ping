/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bcaumont <bcaumont@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/05 10:09:46 by bcaumont          #+#    #+#             */
/*   Updated: 2026/11/05 15:17:41 by bcaumont         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h"

#include "ft_ping.h"

int main(int argc, char **argv)
{
    t_ping ping;
    int    status;

    memset(&ping, 0, sizeof(ping));
    ping.sockfd = -1;
    ping.packet_size = DEFAULT_PAYLOAD_SIZE;
    ping.timeout = 1;
    ping.ttl = 64;
    if (parse_args(argc, argv, &ping) != 0)
        return (EXIT_FAILURE);
    if (ping.help)
    {
        print_usage();
        return (EXIT_SUCCESS);
    }
    if (resolve_target(&ping) != 0)
        return (EXIT_FAILURE);
    if (create_socket(&ping) != 0)
        return (EXIT_FAILURE);
    ping.identifier = (uint16_t)getpid();
    ping.sent_times = calloc(65536, sizeof(*ping.sent_times));
    if (ping.sent_times == NULL)
    {
        perror("ft_ping: calloc");
        close(ping.sockfd);
        return (EXIT_FAILURE);
    }
    status = ping_loop(&ping);
    free(ping.sent_times);
    close(ping.sockfd);
    return (status == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}
