/*
 * main.c — точка входа myRPC-client.
 *
 * Аргументы командной строки:
 *   -c, --command <cmd>   команда bash для выполнения на сервере
 *   -h, --host <ip>       адрес сервера
 *   -p, --port <port>     порт сервера (по умолчанию 1234)
 *   -s, --stream          использовать потоковый сокет TCP (по умолчанию)
 *   -d, --dgram           использовать датаграммный сокет UDP
 *       --help            вывести справку
 */

#include "client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/socket.h>

static void
print_usage (const char *progname)
{
  fprintf (stdout,
           "Usage: %s -c <command> -h <host> [OPTIONS]\n"
           "\n"
           "myRPC client — send a bash command for remote execution.\n"
           "\n"
           "Required:\n"
           "  -c, --command <cmd>   bash command to execute on the server\n"
           "  -h, --host    <ip>    server IP address or hostname\n"
           "\n"
           "Options:\n"
           "  -p, --port <port>     server port (default: 1234)\n"
           "  -s, --stream          use TCP stream socket (default)\n"
           "  -d, --dgram           use UDP datagram socket\n"
           "      --help            show this help and exit\n"
           "\n"
           "Examples:\n"
           "  %s -c \"ls -la /tmp\" -h 192.168.1.10 -p 1234 --stream\n"
           "  %s --command \"uptime\" --host 192.168.1.10 --dgram\n"
           "\n",
           progname, progname, progname);
}

int
main (int argc, char *argv[])
{
  client_opts_t opts;
  int           opt;
  int           has_command = 0;
  int           has_host    = 0;

  static struct option long_options[] = {
    { "command", required_argument, NULL, 'c' },
    { "host",    required_argument, NULL, 'h' },
    { "port",    required_argument, NULL, 'p' },
    { "stream",  no_argument,       NULL, 's' },
    { "dgram",   no_argument,       NULL, 'd' },
    { "help",    no_argument,       NULL,  0  },
    { 0, 0, 0, 0 }
  };

  /* Значения по умолчанию */
  memset (&opts, 0, sizeof (opts));
  opts.port        = 1234;
  opts.socket_type = SOCK_STREAM;
  strncpy (opts.host, "127.0.0.1", sizeof (opts.host) - 1);

  if (argc < 2)
    {
      print_usage (argv[0]);
      return EXIT_FAILURE;
    }

  while (1)
    {
      int option_index = 0;

      opt = getopt_long (argc, argv, "c:h:p:sd", long_options, &option_index);
      if (opt == -1)
        break;

      switch (opt)
        {
        case 0:
          if (strcmp (long_options[option_index].name, "help") == 0)
            {
              print_usage (argv[0]);
              return EXIT_SUCCESS;
            }
          break;

        case 'c':
          strncpy (opts.command, optarg, sizeof (opts.command) - 1);
          has_command = 1;
          break;

        case 'h':
          strncpy (opts.host, optarg, sizeof (opts.host) - 1);
          has_host = 1;
          break;

        case 'p':
          opts.port = atoi (optarg);
          if (opts.port <= 0 || opts.port > 65535)
            {
              fprintf (stderr, "myrpc-client: invalid port: %s\n", optarg);
              return EXIT_FAILURE;
            }
          break;

        case 's':
          opts.socket_type = SOCK_STREAM;
          break;

        case 'd':
          opts.socket_type = SOCK_DGRAM;
          break;

        case '?':
        default:
          fprintf (stderr, "Try '%s --help' for usage.\n", argv[0]);
          return EXIT_FAILURE;
        }
    }

  /* Проверяем обязательные аргументы */
  if (!has_command)
    {
      fprintf (stderr, "myrpc-client: option -c/--command is required\n");
      fprintf (stderr, "Try '%s --help' for usage.\n", argv[0]);
      return EXIT_FAILURE;
    }

  if (!has_host)
    {
      fprintf (stderr, "myrpc-client: option -h/--host is required\n");
      fprintf (stderr, "Try '%s --help' for usage.\n", argv[0]);
      return EXIT_FAILURE;
    }

  return client_run (&opts) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
