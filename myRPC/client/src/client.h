/*
 * client.h — прототипы клиента myRPC-client.
 */

#ifndef CLIENT_H
#define CLIENT_H

#define MAX_HOST_LEN 256

typedef struct
{
  char host[MAX_HOST_LEN];
  int  port;
  int  socket_type;   /* SOCK_STREAM или SOCK_DGRAM */
  char command[4096];
} client_opts_t;

/* Подключается к серверу и выполняет запрос. Возвращает 0 при успехе. */
int client_run (const client_opts_t *opts);

#endif /* CLIENT_H */
