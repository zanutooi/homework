# myRPC — Механизм удалённого вызова команд

Проект реализует клиент-серверную систему удалённого выполнения bash-команд
по сети с использованием сокетов (stream/dgram) на языке Си.

## Структура проекта

```
myRPC/
├── README.md               # Этот файл
├── Makefile                # Общий Makefile (собирает всё)
├── .gitignore
├── client/
│   ├── README.md           # Описание клиента
│   ├── Makefile
│   └── src/
│       ├── main.c          # Точка входа клиента
│       ├── client.c        # Логика подключения и отправки
│       ├── client.h
│       ├── protocol.c      # Формирование JSON-запроса
│       └── protocol.h
└── server/
    ├── README.md           # Описание сервера
    ├── Makefile
    └── src/
        ├── main.c          # Точка входа сервера
        ├── server.c        # Сокет, accept, fork
        ├── server.h
        ├── config.c        # Чтение myRPC.conf и users.conf
        ├── config.h
        ├── protocol.c      # Разбор JSON-запроса
        ├── protocol.h
        ├── executor.c      # Выполнение команды, tmpfile
        └── executor.h
```

## Протокол обмена (JSON)

**Запрос клиента:**
```json
{"login":"username","command":"ls -la"}
```

**Ответ сервера (успех):**
```json
{"code":0,"result":"...stdout..."}
```

**Ответ сервера (ошибка):**
```json
{"code":1,"result":"...stderr или описание..."}
```

## Быстрый старт

```bash
# Сборка всего проекта
make all

# Сборка deb-пакетов
make deb

# Очистка
make clean
```

## Конфигурационные файлы

- `/etc/myRPC/myRPC.conf` — порт и тип сокета сервера
- `/etc/myRPC/users.conf` — whitelist разрешённых пользователей

## Зависимости

- GCC, Make
- Debian packaging tools (`dpkg-deb`)
- ОС: AstraLinux SE 1.7 / Debian-совместимые дистрибутивы

## Ветки Git Flow

- `main` — стабильные релизы
- `develop` — интеграция фич
- `feature/*` — отдельные функции
- `release/*` — подготовка релиза
- `hotfix/*` — срочные исправления
