/*
 * executor.h — выполнение bash-команды в дочернем процессе,
 * вывод направляется в tmpfile.
 */

#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "protocol.h"

/*
 * executor_run — выполняет команду cmd через /bin/sh.
 * stdout пишется в /tmp/myRPC_XXXXXX.stdout
 * stderr пишется в /tmp/myRPC_XXXXXX.stderr
 * Результат (stdout или stderr) записывается в resp.
 * Возвращает 0 при успехе, -1 при ошибке.
 */
int executor_run (const char *cmd, rpc_response_t *resp);

#endif /* EXECUTOR_H */
