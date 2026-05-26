/*
 * protocol.c — клиентская сторона протокола myRPC.
 *
 * Формирует JSON-запрос и разбирает JSON-ответ сервера.
 */

#include "protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
proto_json_escape (const char *src, char *dst, int dstlen)
{
  int j = 0;
  int i;

  for (i = 0; src[i] != '\0' && j < dstlen - 2; i++)
    {
      switch (src[i])
        {
        case '\\': dst[j++] = '\\'; dst[j++] = '\\'; break;
        case '"':  dst[j++] = '\\'; dst[j++] = '"';  break;
        case '\n': dst[j++] = '\\'; dst[j++] = 'n';  break;
        case '\r': dst[j++] = '\\'; dst[j++] = 'r';  break;
        case '\t': dst[j++] = '\\'; dst[j++] = 't';  break;
        default:   dst[j++] = src[i]; break;
        }
    }
  dst[j] = '\0';
}

/*
 * proto_build_request — формирует JSON-строку запроса.
 * Возвращает длину строки или -1 при ошибке.
 */
int
proto_build_request (const rpc_request_t *req, char *buf, int buflen)
{
  char escaped_login[128];
  char escaped_cmd[MAX_CMD_LEN * 2];

  proto_json_escape (req->login,   escaped_login, sizeof (escaped_login));
  proto_json_escape (req->command, escaped_cmd,   sizeof (escaped_cmd));

  return snprintf (buf, buflen,
                   "{\"login\":\"%s\",\"command\":\"%s\"}",
                   escaped_login, escaped_cmd);
}

/*
 * proto_parse_response — разбирает JSON-ответ сервера.
 * Возвращает 0 при успехе, -1 при ошибке разбора.
 */
int
proto_parse_response (const char *buf, rpc_response_t *resp)
{
  const char *p;
  const char *start;
  const char *end;
  char        code_str[16];
  int         len;

  memset (resp, 0, sizeof (*resp));

  /* Ищем "code": N */
  p = strstr (buf, "\"code\"");
  if (p == NULL)
    return -1;
  p += 6; /* strlen("\"code\"") */
  while (*p == ' ' || *p == ':' || *p == ' ')
    p++;
  start = p;
  while (*p >= '0' && *p <= '9')
    p++;
  len = (int)(p - start);
  if (len <= 0 || len >= (int)sizeof (code_str))
    return -1;
  strncpy (code_str, start, len);
  code_str[len] = '\0';
  resp->code = atoi (code_str);

  /* Ищем "result":"..." */
  p = strstr (buf, "\"result\"");
  if (p == NULL)
    return -1;
  p += 8; /* strlen("\"result\"") */
  while (*p == ' ' || *p == ':')
    p++;
  if (*p != '"')
    return -1;
  p++; /* пропускаем открывающую кавычку */

  /* Разворачиваем escape-последовательности при чтении */
  start = p;
  end   = p;
  len   = 0;
  while (*end != '\0' && len < MAX_RESULT_LEN - 1)
    {
      if (*end == '\\' && *(end + 1) != '\0')
        {
          end++;
          switch (*end)
            {
            case 'n':  resp->result[len++] = '\n'; break;
            case 'r':  resp->result[len++] = '\r'; break;
            case 't':  resp->result[len++] = '\t'; break;
            case '"':  resp->result[len++] = '"';  break;
            case '\\': resp->result[len++] = '\\'; break;
            default:   resp->result[len++] = *end; break;
            }
          end++;
        }
      else if (*end == '"')
        {
          break;
        }
      else
        {
          resp->result[len++] = *end++;
        }
    }
  resp->result[len] = '\0';
  (void) start;

  return 0;
}
