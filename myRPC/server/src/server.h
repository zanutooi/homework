/*
 * server.h — прототипы сокетного сервера myRPC-server.
 */

#ifndef SERVER_H
#define SERVER_H

#include "config.h"

/* Запускает сервер: создаёт сокет, слушает, обрабатывает клиентов */
int server_run (const server_config_t *cfg, const user_list_t *ul);

/* Обрабатывает одного клиента (вызывается в дочернем процессе) */
void server_handle_client (int client_fd, const user_list_t *ul);

/* Обрабатывает датаграмный запрос */
void server_handle_dgram (int sock_fd, const user_list_t *ul);

#endif /* SERVER_H */
