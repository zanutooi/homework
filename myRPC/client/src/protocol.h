/*
 * protocol.h — формирование JSON-запроса и разбор JSON-ответа (клиентская сторона).
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#define MAX_CMD_LEN    4096
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

void proto_json_escape   (const char *src, char *dst, int dstlen);
int  proto_build_request (const rpc_request_t *req, char *buf, int buflen);
int  proto_parse_response (const char *buf, rpc_response_t *resp);

#endif /* PROTOCOL_H */
