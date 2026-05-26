/*
 * client.c — установка соединения и отправка запроса серверу myRPC.
 *
 * Поддерживает SOCK_STREAM (TCP) и SOCK_DGRAM (UDP).
 */

#include "client.h"
#include "protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <syslog.h>

#define SEND_BUF_SIZE  (MAX_CMD_LEN * 2 + 256)
#define RECV_BUF_SIZE  (MAX_RESULT_LEN * 2 + 256)

/*
 * client_run — основная функция клиента.
 * Получает логин текущего пользователя, формирует JSON-запрос,
 * отправляет на сервер, выводит ответ на stdout.
 */
int
client_run (const client_opts_t *opts)
{
  int                  sock_fd;
  struct sockaddr_in   server_addr;
  struct hostent      *he;
  char                 send_buf[SEND_BUF_SIZE];
  char                 recv_buf[RECV_BUF_SIZE];
  rpc_request_t        req;
  rpc_response_t       resp;
  const char          *username;
  ssize_t              n;

  /* Определяем логин текущего пользователя */
  username = getenv ("USER");
  if (username == NULL)
    username = getenv ("LOGNAME");
  if (username == NULL)
    username = "unknown";

  /* Формируем запрос */
  memset (&req, 0, sizeof (req));
  strncpy (req.login,   username,       sizeof (req.login) - 1);
  strncpy (req.command, opts->command,  sizeof (req.command) - 1);

  proto_build_request (&req, send_buf, sizeof (send_buf));

  /* Разрешаем хост */
  he = gethostbyname (opts->host);
  if (he == NULL)
    {
      fprintf (stderr, "myrpc-client: cannot resolve host '%s'\n", opts->host);
      return -1;
    }

  /* Создаём сокет */
  sock_fd = socket (AF_INET, opts->socket_type, 0);
  if (sock_fd < 0)
    {
      perror ("myrpc-client: socket");
      return -1;
    }

  memset (&server_addr, 0, sizeof (server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port   = htons ((uint16_t) opts->port);
  memcpy (&server_addr.sin_addr, he->h_addr, he->h_length);

  if (opts->socket_type == SOCK_STREAM)
    {
      /* TCP: подключаемся */
      if (connect (sock_fd, (struct sockaddr *) &server_addr,
                   sizeof (server_addr)) < 0)
        {
          perror ("myrpc-client: connect");
          close (sock_fd);
          return -1;
        }

      /* Отправляем запрос */
      if (send (sock_fd, send_buf, strlen (send_buf), 0) < 0)
        {
          perror ("myrpc-client: send");
          close (sock_fd);
          return -1;
        }

      /* Получаем ответ */
      memset (recv_buf, 0, sizeof (recv_buf));
      n = recv (sock_fd, recv_buf, sizeof (recv_buf) - 1, 0);
      if (n < 0)
        {
          perror ("myrpc-client: recv");
          close (sock_fd);
          return -1;
        }
      recv_buf[n] = '\0';
    }
  else
    {
      /* UDP: отправляем датаграмму */
      if (sendto (sock_fd, send_buf, strlen (send_buf), 0,
                  (struct sockaddr *) &server_addr,
                  sizeof (server_addr)) < 0)
        {
          perror ("myrpc-client: sendto");
          close (sock_fd);
          return -1;
        }

      /* Получаем ответ */
      socklen_t addr_len = sizeof (server_addr);
      memset (recv_buf, 0, sizeof (recv_buf));
      n = recvfrom (sock_fd, recv_buf, sizeof (recv_buf) - 1, 0,
                    (struct sockaddr *) &server_addr, &addr_len);
      if (n < 0)
        {
          perror ("myrpc-client: recvfrom");
          close (sock_fd);
          return -1;
        }
      recv_buf[n] = '\0';
    }

  close (sock_fd);

  /* Разбираем и выводим ответ */
  if (proto_parse_response (recv_buf, &resp) != 0)
    {
      fprintf (stderr, "myrpc-client: invalid response from server:\n%s\n",
               recv_buf);
      return -1;
    }

  if (resp.code == 0)
    {
      printf ("%s", resp.result);
      return 0;
    }
  else
    {
      fprintf (stderr, "myrpc-client: error: %s\n", resp.result);
      return 1;
    }
}
