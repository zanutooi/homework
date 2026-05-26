/*
 * executor.c — выполнение bash-команды с перенаправлением
 * stdout/stderr в временные файлы (mkstemps).
 *
 * Шаблоны: /tmp/myRPC_XXXXXX.stdout  /tmp/myRPC_XXXXXX.stderr
 */

#include "executor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>

#define STDOUT_TEMPLATE "/tmp/myRPC_XXXXXX.stdout"
#define STDERR_TEMPLATE "/tmp/myRPC_XXXXXX.stderr"

/* Читает содержимое файла fd в буфер buf (max buflen байт) */
static int
read_file_to_buf (int fd, char *buf, int buflen)
{
  int total = 0;
  int n;

  lseek (fd, 0, SEEK_SET);
  while (total < buflen - 1)
    {
      n = (int) read (fd, buf + total, buflen - 1 - total);
      if (n <= 0)
        break;
      total += n;
    }
  buf[total] = '\0';
  return total;
}

/*
 * executor_run — запускает cmd через /bin/sh -c.
 * При успехе (exit code 0) resp->code = 0, resp->result = stdout.
 * При ошибке resp->code = 1, resp->result = stderr.
 */
int
executor_run (const char *cmd, rpc_response_t *resp)
{
  char   stdout_path[] = STDOUT_TEMPLATE;
  char   stderr_path[] = STDERR_TEMPLATE;
  int    stdout_fd;
  int    stderr_fd;
  pid_t  pid;
  int    status;
  int    ret = 0;

  memset (resp, 0, sizeof (*resp));

  /* Создаём временные файлы через mkstemps */
  stdout_fd = mkstemps (stdout_path, 7); /* 7 = len(".stdout") */
  if (stdout_fd < 0)
    {
      syslog (LOG_ERR, "executor_run: mkstemps stdout: %s", strerror (errno));
      resp->code = 1;
      snprintf (resp->result, sizeof (resp->result),
                "server error: cannot create stdout tmpfile");
      return -1;
    }

  stderr_fd = mkstemps (stderr_path, 7); /* 7 = len(".stderr") */
  if (stderr_fd < 0)
    {
      syslog (LOG_ERR, "executor_run: mkstemps stderr: %s", strerror (errno));
      close (stdout_fd);
      unlink (stdout_path);
      resp->code = 1;
      snprintf (resp->result, sizeof (resp->result),
                "server error: cannot create stderr tmpfile");
      return -1;
    }

  syslog (LOG_INFO, "executor_run: running cmd='%s'", cmd);
  syslog (LOG_INFO, "executor_run: stdout=%s stderr=%s",
          stdout_path, stderr_path);

  pid = fork ();
  if (pid < 0)
    {
      syslog (LOG_ERR, "executor_run: fork failed: %s", strerror (errno));
      close (stdout_fd);
      close (stderr_fd);
      unlink (stdout_path);
      unlink (stderr_path);
      resp->code = 1;
      snprintf (resp->result, sizeof (resp->result),
                "server error: fork failed");
      return -1;
    }

  if (pid == 0)
    {
      /* Дочерний процесс: перенаправляем stdout и stderr */
      dup2 (stdout_fd, STDOUT_FILENO);
      dup2 (stderr_fd, STDERR_FILENO);
      close (stdout_fd);
      close (stderr_fd);

      execl ("/bin/sh", "sh", "-c", cmd, (char *) NULL);
      /* Если execl вернулся — ошибка */
      _exit (127);
    }

  /* Родительский процесс: ждём завершения дочернего */
  if (waitpid (pid, &status, 0) < 0)
    {
      syslog (LOG_ERR, "executor_run: waitpid: %s", strerror (errno));
      ret = -1;
    }

  if (WIFEXITED (status) && WEXITSTATUS (status) == 0)
    {
      /* Команда выполнена успешно — читаем stdout */
      resp->code = 0;
      read_file_to_buf (stdout_fd, resp->result, sizeof (resp->result));
    }
  else
    {
      /* Ошибка — читаем stderr */
      resp->code = 1;
      if (read_file_to_buf (stderr_fd, resp->result,
                            sizeof (resp->result)) == 0)
        {
          snprintf (resp->result, sizeof (resp->result),
                    "command exited with code %d",
                    WIFEXITED (status) ? WEXITSTATUS (status) : -1);
        }
    }

  close (stdout_fd);
  close (stderr_fd);
  unlink (stdout_path);
  unlink (stderr_path);

  syslog (LOG_INFO, "executor_run: code=%d", resp->code);
  return ret;
}
