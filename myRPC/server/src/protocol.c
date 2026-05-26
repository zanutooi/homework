/*
 * protocol.c — разбор входящих JSON-запросов и формирование JSON-ответов.
 *
 * Намеренно написан без внешних JSON-библиотек: требование задания — чистый Си.
 * Парсер простой (ищет "login" и "command" поля), достаточен для нашего протокола.
 */

#include "protocol.h"

#include <stdio.h>
#include <string.h>
#include <syslog.h>

/*
 * proto_json_escape — экранирует спецсимволы JSON в строке src.
 * Обрабатывает: \, ", \n, \r, \t.
 */
void
proto_json_escape (const char *src, char *dst, int dstlen)
{
  int j = 0;
  int i;

  for (i = 0; src[i] != '\0' && j < dstlen - 2; i++)
    {
      switch (src[i])
        {
        case '\\':
          dst[j++] = '\\'; dst[j++] = '\\'; break;
        case '"':
          dst[j++] = '\\'; dst[j++] = '"';  break;
        case '\n':
          dst[j++] = '\\'; dst[j++] = 'n';  break;
        case '\r':
          dst[j++] = '\\'; dst[j++] = 'r';  break;
        case '\t':
          dst[j++] = '\\'; dst[j++] = 't';  break;
        default:
          dst[j++] = src[i]; break;
        }
    }
  dst[j] = '\0';
}

/*
 * extract_json_string — вспомогательная функция.
 * Ищет в buf значение по ключу key формата "key":"value".
 * Записывает значение в out (max outlen символов).
 * Возвращает 1 при успехе, 0 при ошибке.
 */
static int
extract_json_string (const char *buf, const char *key,
                     char *out, int outlen)
{
  char search[128];
  const char *pos;
  const char *start;
  const char *end;
  int len;

  snprintf (search, sizeof (search), "\"%s\"", key);
  pos = strstr (buf, search);
  if (pos == NULL)
    return 0;

  /* Пропускаем "key": */
  pos += strlen (search);
  while (*pos == ' ' || *pos == ':' || *pos == ' ')
    pos++;

  if (*pos != '"')
    return 0;
  pos++; /* пропускаем открывающую кавычку */

  start = pos;
  /* Ищем закрывающую кавычку, учитываем экранирование */
  end = start;
  while (*end != '\0')
    {
      if (*end == '\\' && *(end + 1) != '\0')
        {
          end += 2;
          continue;
        }
      if (*end == '"')
        break;
      end++;
    }

  if (*end != '"')
    return 0;

  len = (int)(end - start);
  if (len >= outlen)
    len = outlen - 1;

  strncpy (out, start, len);
  out[len] = '\0';
  return 1;
}

/*
 * proto_parse_request — разбирает JSON-запрос из buf в структуру req.
 * Ожидаемый формат: {"login":"user","command":"cmd"}
 * Возвращает 0 при успехе, -1 при ошибке.
 */
int
proto_parse_request (const char *buf, rpc_request_t *req)
{
  memset (req, 0, sizeof (*req));

  if (!extract_json_string (buf, "login", req->login, sizeof (req->login)))
    {
      syslog (LOG_WARNING, "proto_parse_request: missing 'login' field");
      return -1;
    }

  if (!extract_json_string (buf, "command", req->command, sizeof (req->command)))
    {
      syslog (LOG_WARNING, "proto_parse_request: missing 'command' field");
      return -1;
    }

  return 0;
}

/*
 * proto_build_response — формирует JSON-ответ из resp в buf.
 * Возвращает длину записанной строки или -1 при ошибке.
 */
int
proto_build_response (const rpc_response_t *resp, char *buf, int buflen)
{
  char escaped[MAX_RESULT_LEN * 2];

  proto_json_escape (resp->result, escaped, sizeof (escaped));
  return snprintf (buf, buflen,
                   "{\"code\":%d,\"result\":\"%s\"}",
                   resp->code, escaped);
}
