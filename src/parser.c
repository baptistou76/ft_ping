/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bcaumont <bcaumont@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/05 15:14:00 by bcaumont          #+#    #+#             */
/*   Updated: 2026/11/05 15:14:01 by bcaumont         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ping.h"

static int parse_integer(const char *text, int base, int min, int max,
        int *value)
{
    char *end;
    long parsed;

    if (text == NULL || *text == '\0')
        return (-1);
    errno = 0;
    parsed = strtol(text, &end, base);
    if (errno != 0 || *end != '\0' || parsed < min || parsed > max)
        return (-1);
    *value = (int)parsed;
    return (0);
}

static int option_value(int argc, char **argv, int *index,
        const char *option, int base, int min, int max, int *destination)
{
    if (*index + 1 >= argc
        || parse_integer(argv[*index + 1], base, min, max, destination) != 0)
    {
        fprintf(stderr, "ft_ping: invalid or missing value for %s\n", option);
        return (-1);
    }
    (*index)++;
    return (0);
}

static int parse_pattern(const char *value)
{
    size_t i;

    if (*value == '\0' || strlen(value) > PATTERN_MAX_SIZE * 2
        || strlen(value) % 2 != 0)
        return (-1);
    i = 0;
    while (value[i] != '\0')
    {
        if (!((value[i] >= '0' && value[i] <= '9')
                || (value[i] >= 'a' && value[i] <= 'f')
                || (value[i] >= 'A' && value[i] <= 'F')))
            return (-1);
        i++;
    }
    return (0);
}

static int parse_timestamp_mode(t_ping *ping, const char *value)
{
    if (strcmp(value, "tsonly") == 0)
        ping->timestamp_mode = TS_TSONLY;
    else if (strcmp(value, "tsaddr") == 0)
        ping->timestamp_mode = TS_TSADDR;
    else
    {
        fprintf(stderr, "ft_ping: --ip-timestamp expects tsonly or tsaddr\n");
        return (-1);
    }
    ping->ip_timestamp = 1;
    return (0);
}

static int parse_long_option(t_ping *ping, const char *arg)
{
    const char *value;

    if (strncmp(arg, "--ttl=", 6) == 0)
    {
        value = arg + 6;
        if (parse_integer(value, 10, 1, 255, &ping->ttl) != 0)
            return (-1);
        return (1);
    }
    if (strncmp(arg, "--ip-timestamp=", 15) == 0)
    {
        value = arg + 15;
        if (parse_timestamp_mode(ping, value) != 0)
            return (-1);
        return (1);
    }
    return (0);
}

int parse_args(int argc, char **argv, t_ping *ping)
{
    int i;
    int long_option;

    i = 1;
    while (i < argc)
    {
        long_option = parse_long_option(ping, argv[i]);
        if (long_option < 0)
            return (-1);
        if (long_option > 0)
            ;
        else if (strcmp(argv[i], "-?") == 0 || strcmp(argv[i], "--help") == 0)
            ping->help = 1;
        else if (strcmp(argv[i], "-v") == 0)
            ping->verbose = 1;
        else if (strcmp(argv[i], "-f") == 0)
            ping->flood = 1;
        else if (strcmp(argv[i], "-n") == 0)
            ping->numeric = 1;
        else if (strcmp(argv[i], "-r") == 0)
            ping->ignore_route = 1;
        else if (strcmp(argv[i], "-c") == 0)
        {
            if (option_value(argc, argv, &i, "-c", 10, 1, INT_MAX,
                    &ping->count) != 0)
                return (-1);
        }
        else if (strcmp(argv[i], "-l") == 0)
        {
            if (option_value(argc, argv, &i, "-l", 10, 0, INT_MAX,
                    &ping->preload) != 0)
                return (-1);
        }
        else if (strcmp(argv[i], "-w") == 0)
        {
            if (option_value(argc, argv, &i, "-w", 10, 1, INT_MAX,
                    &ping->deadline) != 0)
                return (-1);
        }
        else if (strcmp(argv[i], "-W") == 0)
        {
            if (option_value(argc, argv, &i, "-W", 10, 1, 3600,
                    &ping->timeout) != 0)
                return (-1);
        }
        else if (strcmp(argv[i], "-s") == 0)
        {
            if (option_value(argc, argv, &i, "-s", 10, 0, MAX_PAYLOAD_SIZE,
                    &ping->packet_size) != 0)
                return (-1);
        }
        else if (strcmp(argv[i], "-T") == 0)
        {
            if (option_value(argc, argv, &i, "-T", 0, 0, 255,
                    &ping->tos) != 0)
                return (-1);
        }
        else if (strcmp(argv[i], "--ttl") == 0)
        {
            if (option_value(argc, argv, &i, "--ttl", 10, 1, 255,
                    &ping->ttl) != 0)
                return (-1);
        }
        else if (strcmp(argv[i], "--ip-timestamp") == 0)
        {
            if (i + 1 >= argc || parse_timestamp_mode(ping, argv[i + 1]) != 0)
                return (-1);
            i++;
        }
        else if (strcmp(argv[i], "-p") == 0)
        {
            if (i + 1 >= argc || parse_pattern(argv[i + 1]) != 0)
            {
                fprintf(stderr, "ft_ping: -p expects 1 to 16 bytes in hexadecimal\n");
                return (-1);
            }
            ping->pattern = argv[++i];
        }
        else if (argv[i][0] == '-')
        {
            fprintf(stderr, "ft_ping: unknown option '%s'\n", argv[i]);
            return (-1);
        }
        else if (ping->target != NULL)
        {
            fprintf(stderr, "ft_ping: only one destination is allowed\n");
            return (-1);
        }
        else
            ping->target = argv[i];
        i++;
    }
    if (!ping->help && ping->target == NULL)
    {
        fprintf(stderr, "ft_ping: missing destination address\n");
        return (-1);
    }
    if (ping->flood && ping->preload > 0)
        fprintf(stderr, "ft_ping: -l is ignored in flood mode\n");
    return (0);
}

void print_usage(void)
{
    printf("Usage: ft_ping [OPTION...] HOST\n"
        "Send ICMP ECHO_REQUEST packets to network hosts.\n\n"
        "  -v                 verbose output\n"
        "  -? --help          display this help\n"
        "  -c N               stop after N requests (testing extension)\n"
        "  -f                 flood ping\n"
        "  -l N               preload N packets\n"
        "  -n                 numeric output (no reverse DNS)\n"
        "  -w N               stop after N seconds\n"
        "  -W N               maximum response wait in seconds\n"
        "  -p HEX             payload pattern (up to 16 bytes)\n"
        "  -r                 bypass normal routing table\n"
        "  -s N               payload size in bytes\n"
        "  -T NUM             set IP type of service (0..255)\n"
        "  --ttl N            set IPv4 TTL (1..255)\n"
        "  --ip-timestamp MODE  IP timestamp: tsonly or tsaddr\n");
}
