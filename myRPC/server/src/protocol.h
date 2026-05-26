/*
 * protocol.h — разбор и формирование JSON-сообщений протокола myRPC.
 *
 * Формат запроса:
 *   {"login":"имя","command":"команда bash"}
 *
 * Формат ответа:
 *   {"code":0,"result":"..."}
 *   {"code":1,"result":"...ошибка..."}
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#define MAX_CMD_LEN   4096
#define MAX_RESULT_LEN 65536

typedef struct
{
  char login[64];
  char command[MAX_CMD_LEN];
} rpc_request_t;

typedef struct
{
  int  code;
  char result[MAX_RESULT_LEN];
} rpc_response_t;

int  proto_parse_request  (const char *buf, rpc_request_t *req);
int  proto_build_response (const rpc_response_t *resp, char *buf, int buflen);

/* Экранирование спецсимволов для JSON */
void proto_json_escape (const char *src, char *dst, int dstlen);

#endif /* PROTOCOL_H */
