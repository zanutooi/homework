# myRPC-server

Программа-демон, принимающая JSON-запросы от myRPC-client,
проверяющая пользователя по whitelist и выполняющая bash-команды.

## Сборка

```bash
cd server
make all
# Бинарник: ../dist/myrpc-server
```

## Установка (deb-пакет)

```bash
make deb
sudo dpkg -i ../dist/myrpc-server_1.0.0_amd64.deb
```

При установке deb-пакета автоматически:
- устанавливаются конфигурационные файлы в `/etc/myRPC/`
- регистрируется и запускается systemd-служба `myrpc-server`

## Конфигурационные файлы

### `/etc/myRPC/myRPC.conf`

```ini
# Порт сервера
port = 1234

# Тип сокета: stream (TCP) или dgram (UDP)
socket_type = stream
```

### `/etc/myRPC/users.conf`

```
# Whitelist разрешённых пользователей (по одному на строку)
alice
bob
```

## Запуск

```bash
# Как обычное консольное приложение
myrpc-server

# Как демон (в фоне)
myrpc-server --daemon

# С логом в файл
myrpc-server --log /var/log/myrpc.log

# Через systemd
systemctl start myrpc-server
systemctl status myrpc-server
```

## Управление

```bash
# Перечитать конфиг без перезапуска (SIGHUP)
kill -HUP $(pidof myrpc-server)
# или
systemctl reload myrpc-server

# Остановить (SIGTERM — сначала дочерние процессы, потом демон)
systemctl stop myrpc-server
```

## Сигналы

| Сигнал    | Действие                                      |
|-----------|-----------------------------------------------|
| `SIGINT`  | Корректное завершение (ждёт дочерние процессы) |
| `SIGTERM` | Корректное завершение (ждёт дочерние процессы) |
| `SIGHUP`  | Перечитать конфигурацию (без перезапуска)     |
| `SIGCHLD` | Подобрать завершённые дочерние процессы       |

## Логирование

Сервер пишет в `syslog` (facility `LOG_DAEMON`). Смотреть:

```bash
journalctl -u myrpc-server -f
# или
tail -f /var/log/syslog | grep myrpc
```

## Архитектура

```
main()
 ├── config_load()      читает myRPC.conf
 ├── users_load()       читает users.conf
 ├── setup_signals()    SIGINT/SIGTERM/SIGHUP/SIGCHLD
 ├── daemonize()        (если --daemon)
 └── server_run()
      └── accept() / recvfrom()
           └── fork()
                └── server_handle_client()
                     ├── proto_parse_request()
                     ├── user_allowed()
                     └── executor_run()
                          ├── mkstemps() → /tmp/myRPC_XXXXXX.stdout
                          ├── mkstemps() → /tmp/myRPC_XXXXXX.stderr
                          └── fork() + execl("/bin/sh", "-c", cmd)
```
