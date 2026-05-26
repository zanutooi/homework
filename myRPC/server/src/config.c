/*
 * config.c — чтение конфигурационных файлов myRPC-server.
 *
 * Формат myRPC.conf:
 *   # комментарий
 *   port = 1234
 *   socket_type = stream   (или dgram)
 *
 * Формат users.conf:
 *   alice
 *   bob
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/socket.h>

/* Обрезаем пробелы и перевод строки в конце */
static void
rtrim (char *s)
{
  int len = (int) strlen (s);
  while (len > 0
         && (s[len - 1] == '\n' || s[len - 1] == '\r'
             || s[len - 1] == ' '  || s[len - 1] == '\t'))
    {
      s[--len] = '\0';
    }
}

/* Обрезаем пробелы в начале */
static char *
ltrim (char *s)
{
  while (*s == ' ' || *s == '\t')
    s++;
  return s;
}

/*
 * config_load — загружает server_config_t из файла path.
 * Возвращает 0 при успехе, -1 при ошибке.
 */
int
config_load (const char *path, server_config_t *cfg)
{
  FILE *fp;
  char  line[MAX_LINE_LEN];
  char  key[MAX_LINE_LEN];
  char  value[MAX_LINE_LEN];

  /* Значения по умолчанию */
  cfg->port        = DEFAULT_PORT;
  cfg->socket_type = DEFAULT_SOCK;

  fp = fopen (path, "r");
  if (fp == NULL)
    {
      syslog (LOG_WARNING, "config_load: cannot open %s, using defaults", path);
      return -1;
    }

  while (fgets (line, sizeof (line), fp) != NULL)
    {
      char *p = ltrim (line);

      /* Пропускаем пустые строки и комментарии */
      if (*p == '#' || *p == '\0' || *p == '\n')
        continue;

      if (sscanf (p, "%[^=]=%s", key, value) != 2)
        continue;

      /* Убираем пробелы вокруг ключа */
      rtrim (key);
      char *k = ltrim (key);
      char *v = ltrim (value);
      rtrim (v);

      if (strcmp (k, "port") == 0)
        {
          cfg->port = atoi (v);
        }
      else if (strcmp (k, "socket_type") == 0)
        {
          if (strcmp (v, "dgram") == 0)
            cfg->socket_type = SOCK_DGRAM;
          else
            cfg->socket_type = SOCK_STREAM;
        }
    }

  fclose (fp);
  syslog (LOG_INFO, "config_load: port=%d, socket_type=%s",
          cfg->port,
          cfg->socket_type == SOCK_DGRAM ? "dgram" : "stream");
  return 0;
}

/*
 * users_load — загружает список разрешённых пользователей из файла path.
 * Возвращает 0 при успехе, -1 при ошибке.
 */
int
users_load (const char *path, user_list_t *ul)
{
  FILE *fp;
  char  line[MAX_LINE_LEN];

  ul->count = 0;

  fp = fopen (path, "r");
  if (fp == NULL)
    {
      syslog (LOG_WARNING, "users_load: cannot open %s", path);
      return -1;
    }

  while (fgets (line, sizeof (line), fp) != NULL
         && ul->count < MAX_USERS)
    {
      char *p = ltrim (line);
      if (*p == '#' || *p == '\0' || *p == '\n')
        continue;

      rtrim (p);
      if (strlen (p) == 0)
        continue;

      strncpy (ul->users[ul->count], p, MAX_USERNAME_LEN - 1);
      ul->users[ul->count][MAX_USERNAME_LEN - 1] = '\0';
      ul->count++;
    }

  fclose (fp);
  syslog (LOG_INFO, "users_load: loaded %d user(s)", ul->count);
  return 0;
}

/*
 * user_allowed — проверяет, есть ли username в whitelist.
 * Возвращает 1 если разрешён, 0 если нет.
 */
int
user_allowed (const user_list_t *ul, const char *username)
{
  int i;
  for (i = 0; i < ul->count; i++)
    {
      if (strcmp (ul->users[i], username) == 0)
        return 1;
    }
  return 0;
}
