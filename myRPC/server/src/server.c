/*
 * server.c — сокетный сервер myRPC-server.
 *
 * Для SOCK_STREAM: accept() + fork() на каждого клиента.
 * Для SOCK_DGRAM:  recvfrom() в основном процессе (датаграммы атомарны).
 *
 * Стиль кода: GNU Coding Standards.
 */

#include "server.h"
#include "protocol.h"
#include "executor.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define RECV_BUF_SIZE  8192
#define SEND_BUF_SIZE  (MAX_RESULT_LEN * 2 + 128)

/* Глобальный флаг для корректного завершения */
extern volatile sig_atomic_t g_running;

/*
 * server_handle_client — обрабатывает одного TCP-клиента.
 * Вызывается в дочернем процессе после fork().
 */
void
server_handle_client (int client_fd, const user_list_t *ul)
{
  char           recv_buf[RECV_BUF_SIZE];
  char           send_buf[SEND_BUF_SIZE];
  rpc_request_t  req;
  rpc_response_t resp;
  ssize_t        n;

  memset (recv_buf, 0, sizeof (recv_buf));

  n = recv (client_fd, recv_buf, sizeof (recv_buf) - 1, 0);
  if (n <= 0)
    {
      syslog (LOG_WARNING, "handle_client: recv failed: %s", strerror (errno));
      close (client_fd);
      return;
    }
  recv_buf[n] = '\0';
  syslog (LOG_INFO, "handle_client: received %zd bytes", n);

  /* Разбираем запрос */
  if (proto_parse_request (recv_buf, &req) != 0)
    {
      resp.code = 1;
      snprintf (resp.result, sizeof (resp.result), "invalid request format");
    }
  else
    {
      syslog (LOG_INFO, "handle_client: login='%s' command='%s'",
              req.login, req.command);

      if (!user_allowed (ul, req.login))
        {
          syslog (LOG_WARNING, "handle_client: user '%s' not in whitelist",
                  req.login);
          resp.code = 1;
          snprintf (resp.result, sizeof (resp.result),
                    "user '%s' is not allowed", req.login);
        }
      else
        {
          executor_run (req.command, &resp);
        }
    }

  proto_build_response (&resp, send_buf, sizeof (send_buf));
  send (client_fd, send_buf, strlen (send_buf), 0);
  close (client_fd);
}

/*
 * server_handle_dgram — обрабатывает один UDP-датаграммный запрос.
 */
void
server_handle_dgram (int sock_fd, const user_list_t *ul)
{
  char                recv_buf[RECV_BUF_SIZE];
  char                send_buf[SEND_BUF_SIZE];
  rpc_request_t       req;
  rpc_response_t      resp;
  struct sockaddr_in  client_addr;
  socklen_t           client_len = sizeof (client_addr);
  ssize_t             n;

  memset (recv_buf, 0, sizeof (recv_buf));
  memset (&client_addr, 0, sizeof (client_addr));

  n = recvfrom (sock_fd, recv_buf, sizeof (recv_buf) - 1, 0,
                (struct sockaddr *) &client_addr, &client_len);
  if (n <= 0)
    return;
  recv_buf[n] = '\0';

  if (proto_parse_request (recv_buf, &req) != 0)
    {
      resp.code = 1;
      snprintf (resp.result, sizeof (resp.result), "invalid request format");
    }
  else
    {
      syslog (LOG_INFO, "dgram: login='%s' command='%s'",
              req.login, req.command);

      if (!user_allowed (ul, req.login))
        {
          syslog (LOG_WARNING, "dgram: user '%s' not in whitelist", req.login);
          resp.code = 1;
          snprintf (resp.result, sizeof (resp.result),
                    "user '%s' is not allowed", req.login);
        }
      else
        {
          executor_run (req.command, &resp);
        }
    }

  proto_build_response (&resp, send_buf, sizeof (send_buf));
  sendto (sock_fd, send_buf, strlen (send_buf), 0,
          (struct sockaddr *) &client_addr, client_len);
}

/*
 * server_run — основной цикл сервера.
 * Создаёт сокет, bind, listen (для stream), принимает подключения.
 * Каждое новое TCP-подключение обрабатывается в отдельном процессе.
 */
int
server_run (const server_config_t *cfg, const user_list_t *ul)
{
  int                sock_fd;
  int                opt = 1;
  struct sockaddr_in addr;

  sock_fd = socket (AF_INET, cfg->socket_type, 0);
  if (sock_fd < 0)
    {
      syslog (LOG_ERR, "server_run: socket: %s", strerror (errno));
      return -1;
    }

  setsockopt (sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));

  memset (&addr, 0, sizeof (addr));
  addr.sin_family      = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port        = htons ((uint16_t) cfg->port);

  if (bind (sock_fd, (struct sockaddr *) &addr, sizeof (addr)) < 0)
    {
      syslog (LOG_ERR, "server_run: bind port %d: %s",
              cfg->port, strerror (errno));
      close (sock_fd);
      return -1;
    }

  if (cfg->socket_type == SOCK_STREAM)
    {
      if (listen (sock_fd, 10) < 0)
        {
          syslog (LOG_ERR, "server_run: listen: %s", strerror (errno));
          close (sock_fd);
          return -1;
        }
      syslog (LOG_INFO, "server_run: listening TCP on port %d", cfg->port);

      /* Основной цикл приёма TCP-подключений */
      while (g_running)
        {
          struct sockaddr_in client_addr;
          socklen_t          client_len = sizeof (client_addr);
          int                client_fd;
          pid_t              pid;

          client_fd = accept (sock_fd,
                              (struct sockaddr *) &client_addr,
                              &client_len);
          if (client_fd < 0)
            {
              if (errno == EINTR)
                continue; /* прерван сигналом — проверяем g_running */
              syslog (LOG_ERR, "server_run: accept: %s", strerror (errno));
              continue;
            }

          syslog (LOG_INFO, "server_run: connection from %s:%d",
                  inet_ntoa (client_addr.sin_addr),
                  ntohs (client_addr.sin_port));

          pid = fork ();
          if (pid < 0)
            {
              syslog (LOG_ERR, "server_run: fork: %s", strerror (errno));
              close (client_fd);
              continue;
            }

          if (pid == 0)
            {
              /* Дочерний процесс */
              close (sock_fd);
              server_handle_client (client_fd, ul);
              exit (EXIT_SUCCESS);
            }

          /* Родительский процесс — закрываем клиентский fd */
          close (client_fd);
        }
    }
  else
    {
      /* SOCK_DGRAM — основной цикл датаграммного сервера */
      syslog (LOG_INFO, "server_run: listening UDP on port %d", cfg->port);
      while (g_running)
        {
          server_handle_dgram (sock_fd, ul);
        }
    }

  close (sock_fd);
  return 0;
}
