/*
 * config.h — структуры и прототипы для чтения конфигурации myRPC-server.
 *
 * Конфигурационные файлы:
 *   /etc/myRPC/myRPC.conf  — порт и тип сокета
 *   /etc/myRPC/users.conf  — whitelist пользователей
 */

#ifndef CONFIG_H
#define CONFIG_H

#define CONFIG_PATH      "/etc/myRPC/myRPC.conf"
#define USERS_PATH       "/etc/myRPC/users.conf"

#define DEFAULT_PORT     1234
#define DEFAULT_SOCK     SOCK_STREAM

#define MAX_USERNAME_LEN 64
#define MAX_USERS        256
#define MAX_LINE_LEN     256

#include <sys/socket.h>

/* Конфигурация сервера, загружаемая из myRPC.conf */
typedef struct
{
  int port;
  int socket_type;   /* SOCK_STREAM или SOCK_DGRAM */
} server_config_t;

/* Whitelist пользователей из users.conf */
typedef struct
{
  char users[MAX_USERS][MAX_USERNAME_LEN];
  int  count;
} user_list_t;

int  config_load (const char *path, server_config_t *cfg);
int  users_load  (const char *path, user_list_t *ul);
int  user_allowed (const user_list_t *ul, const char *username);

#endif /* CONFIG_H */
