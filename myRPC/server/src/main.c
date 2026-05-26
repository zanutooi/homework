/*
 * main.c — точка входа myRPC-server.
 *
 * Обработка сигналов:
 *   SIGINT / SIGTERM — завершить сервер (после дочерних процессов)
 *   SIGHUP           — перечитать конфигурацию
 *   SIGCHLD          — подобрать завершённые дочерние процессы
 *
 * Аргументы командной строки:
 *   -d / --daemon         — запустить как демон
 *   -l / --log <file>     — писать ошибки в файл (иначе syslog)
 *   --help                — справка
 */

#include "server.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/wait.h>

/* Флаг — сервер продолжает работу */
volatile sig_atomic_t g_running = 1;

/* Флаг — нужно перечитать конфиг */
volatile sig_atomic_t g_reload  = 0;

/* Глобальные конфиг и whitelist (для SIGHUP) */
static server_config_t g_cfg;
static user_list_t     g_users;

/* ------------------------------------------------------------------ */
/*  Обработчики сигналов                                               */
/* ------------------------------------------------------------------ */

static void
sig_handler (int signo)
{
  switch (signo)
    {
    case SIGINT:
    case SIGTERM:
      syslog (LOG_INFO, "signal %d received, shutting down", signo);
      g_running = 0;
      break;

    case SIGHUP:
      syslog (LOG_INFO, "SIGHUP received, reloading config");
      g_reload = 1;
      break;

    case SIGCHLD:
      /* Подбираем все завершённые дочерние процессы */
      while (waitpid (-1, NULL, WNOHANG) > 0)
        ;
      syslog (LOG_DEBUG, "SIGCHLD: child reaped");
      break;

    default:
      break;
    }
}

static void
setup_signals (void)
{
  struct sigaction sa;
  memset (&sa, 0, sizeof (sa));
  sa.sa_handler = sig_handler;
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  sigaction (SIGINT,  &sa, NULL);
  sigaction (SIGTERM, &sa, NULL);
  sigaction (SIGHUP,  &sa, NULL);
  sigaction (SIGCHLD, &sa, NULL);
}

/* ------------------------------------------------------------------ */
/*  Демонизация                                                        */
/* ------------------------------------------------------------------ */

static int
daemonize (void)
{
  pid_t pid;

  /* Первый fork */
  pid = fork ();
  if (pid < 0)
    return -1;
  if (pid > 0)
    exit (EXIT_SUCCESS); /* завершаем родителя */

  /* Новая сессия */
  if (setsid () < 0)
    return -1;

  /* Второй fork — гарантируем, что не приобретём управляющий терминал */
  pid = fork ();
  if (pid < 0)
    return -1;
  if (pid > 0)
    exit (EXIT_SUCCESS);

  /* Устанавливаем рабочий каталог */
  chdir ("/");

  /* Закрываем стандартные дескрипторы */
  close (STDIN_FILENO);
  close (STDOUT_FILENO);
  close (STDERR_FILENO);

  /* Перенаправляем в /dev/null */
  open ("/dev/null", O_RDONLY);  /* stdin  */
  open ("/dev/null", O_WRONLY);  /* stdout */
  open ("/dev/null", O_WRONLY);  /* stderr */

  return 0;
}

/* ------------------------------------------------------------------ */
/*  Справка                                                            */
/* ------------------------------------------------------------------ */

static void
print_usage (const char *progname)
{
  fprintf (stdout,
           "Usage: %s [OPTIONS]\n"
           "\n"
           "myRPC server — daemon for remote bash command execution.\n"
           "\n"
           "Options:\n"
           "  -d, --daemon         Run as a background daemon\n"
           "  -l, --log <file>     Log errors to file instead of syslog\n"
           "      --help           Show this help message and exit\n"
           "\n"
           "Config files:\n"
           "  %s   — port and socket type\n"
           "  %s   — whitelist of allowed users\n"
           "\n",
           progname, CONFIG_PATH, USERS_PATH);
}

/* ------------------------------------------------------------------ */
/*  main                                                               */
/* ------------------------------------------------------------------ */

int
main (int argc, char *argv[])
{
  int   opt;
  int   run_as_daemon = 0;
  char *log_file      = NULL;
  FILE *log_fp        = NULL;

  static struct option long_options[] = {
    { "daemon", no_argument,       NULL, 'd' },
    { "log",    required_argument, NULL, 'l' },
    { "help",   no_argument,       NULL,  0  },
    { 0, 0, 0, 0 }
  };

  /* Разбор аргументов командной строки */
  while (1)
    {
      int option_index = 0;

      opt = getopt_long (argc, argv, "dl:", long_options, &option_index);
      if (opt == -1)
        break;

      switch (opt)
        {
        case 0:
          /* Длинная опция без короткого эквивалента */
          if (strcmp (long_options[option_index].name, "help") == 0)
            {
              print_usage (argv[0]);
              return EXIT_SUCCESS;
            }
          break;

        case 'd':
          run_as_daemon = 1;
          break;

        case 'l':
          log_file = optarg;
          break;

        case '?':
        default:
          fprintf (stderr, "Try '%s --help' for usage.\n", argv[0]);
          return EXIT_FAILURE;
        }
    }

  /* Открываем лог */
  if (log_file != NULL)
    {
      log_fp = fopen (log_file, "a");
      if (log_fp == NULL)
        {
          fprintf (stderr, "Cannot open log file '%s': %s\n",
                   log_file, strerror (errno));
          return EXIT_FAILURE;
        }
    }

  /* Инициализируем syslog */
  openlog ("myrpc-server", LOG_PID | LOG_CONS, LOG_DAEMON);
  syslog (LOG_INFO, "myrpc-server starting");

  /* Читаем конфигурацию */
  config_load (CONFIG_PATH, &g_cfg);
  users_load  (USERS_PATH,  &g_users);

  /* Устанавливаем обработчики сигналов */
  setup_signals ();

  /* Демонизация, если запрошена */
  if (run_as_daemon)
    {
      if (daemonize () != 0)
        {
          syslog (LOG_ERR, "daemonize failed: %s", strerror (errno));
          closelog ();
          return EXIT_FAILURE;
        }
      syslog (LOG_INFO, "running as daemon, PID=%d", getpid ());
    }

  /* Основной цикл */
  while (g_running)
    {
      if (g_reload)
        {
          g_reload = 0;
          syslog (LOG_INFO, "reloading configuration");
          config_load (CONFIG_PATH, &g_cfg);
          users_load  (USERS_PATH,  &g_users);
        }

      server_run (&g_cfg, &g_users);

      /* server_run вернулся — либо g_running = 0, либо ошибка */
      if (g_running && !g_reload)
        break;
    }

  /* Ждём завершения всех дочерних процессов перед выходом */
  syslog (LOG_INFO, "waiting for child processes to finish...");
  while (waitpid (-1, NULL, 0) > 0)
    ;

  syslog (LOG_INFO, "myrpc-server stopped");
  closelog ();

  if (log_fp != NULL)
    fclose (log_fp);

  return EXIT_SUCCESS;
}
