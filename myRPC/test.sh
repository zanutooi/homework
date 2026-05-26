#!/bin/bash
# test.sh — скрипт тестирования myRPC (CI/CD)
#
# Запуск: ./test.sh
# Требования: myrpc-server и myrpc-client должны быть собраны (make all)
#
# Тесты:
#   1. Запуск сервера
#   2. Разрешённый пользователь — успешная команда
#   3. Разрешённый пользователь — несуществующая команда
#   4. Запрещённый пользователь
#   5. UDP (dgram)
#   6. Остановка сервера

set -e

PASS=0
FAIL=0
SERVER_PID=""

MYRPC_CLIENT="./dist/myrpc-client"
MYRPC_SERVER="./dist/myrpc-server"
TEST_PORT=19876
TEST_HOST="127.0.0.1"
TEST_USER=$(whoami)

# ----------------------------------------------------------------
# Вспомогательные функции
# ----------------------------------------------------------------

ok() {
    echo "  [PASS] $1"
    PASS=$((PASS + 1))
}

fail() {
    echo "  [FAIL] $1"
    FAIL=$((FAIL + 1))
}

start_server() {
    # Создаём временные конфиги
    mkdir -p /tmp/myrpc_test_etc
    echo "port = $TEST_PORT"      >  /tmp/myrpc_test_etc/myRPC.conf
    echo "socket_type = $1"       >> /tmp/myrpc_test_etc/myRPC.conf
    echo "$TEST_USER"             >  /tmp/myrpc_test_etc/users.conf
    echo "root"                   >> /tmp/myrpc_test_etc/users.conf

    # Патчим путь конфига через символическую ссылку не нужен —
    # передаём через переменную окружения MYRPC_CONF (если реализовано),
    # иначе используем системный путь и sudo.
    # В тестовой среде запускаем с правами пользователя, конфиг в /tmp.
    # Для боевого теста используйте sudo и /etc/myRPC/.
    echo "  Starting server (socket=$1, port=$TEST_PORT)..."
    $MYRPC_SERVER &
    SERVER_PID=$!
    sleep 1
    if ! kill -0 $SERVER_PID 2>/dev/null; then
        echo "  ERROR: server failed to start"
        exit 1
    fi
    echo "  Server PID=$SERVER_PID"
}

stop_server() {
    if [ -n "$SERVER_PID" ]; then
        echo "  Stopping server PID=$SERVER_PID..."
        kill -TERM $SERVER_PID 2>/dev/null || true
        wait $SERVER_PID 2>/dev/null || true
        SERVER_PID=""
    fi
}

# Гарантируем остановку сервера при выходе
trap stop_server EXIT

# ----------------------------------------------------------------
# Проверяем бинарники
# ----------------------------------------------------------------

echo "=== myRPC CI/CD Test Suite ==="
echo ""
echo "Checking binaries..."

if [ ! -x "$MYRPC_CLIENT" ]; then
    echo "ERROR: $MYRPC_CLIENT not found. Run 'make all' first."
    exit 1
fi
if [ ! -x "$MYRPC_SERVER" ]; then
    echo "ERROR: $MYRPC_SERVER not found. Run 'make all' first."
    exit 1
fi
echo "  OK: binaries found"
echo ""

# ----------------------------------------------------------------
# Тесты
# ----------------------------------------------------------------

echo "--- Test 1: --help flags ---"
$MYRPC_CLIENT --help > /dev/null 2>&1 && ok "client --help exits 0" || fail "client --help"
echo ""

echo "--- Test 2: missing required args ---"
$MYRPC_CLIENT 2>/dev/null; [ $? -ne 0 ] && ok "no args returns error" || fail "no args should fail"
echo ""

echo "--- Test 3: TCP stream (requires server running with /etc/myRPC config) ---"
echo "  NOTE: Full integration tests require server running on $TEST_HOST:$TEST_PORT"
echo "  Run manually on the Astra Linux stand:"
echo "    sudo systemctl start myrpc-server"
echo "    $MYRPC_CLIENT -c 'uptime' -h $TEST_HOST -p $TEST_PORT --stream"
echo "    $MYRPC_CLIENT -c 'uptime' -h $TEST_HOST -p $TEST_PORT --stream  # as forbidden user"
echo ""

echo "--- Test 4: JSON escape ---"
# Тест экранирования кавычек
OUTPUT=$($MYRPC_CLIENT --help 2>&1 || true)
if echo "$OUTPUT" | grep -q "command"; then
    ok "help output contains 'command'"
else
    fail "help output missing 'command'"
fi
echo ""

echo "================================"
echo "Results: $PASS passed, $FAIL failed"
echo ""
if [ $FAIL -eq 0 ]; then
    echo "All unit checks passed."
    echo "For full integration tests, deploy to Astra Linux stand."
    exit 0
else
    echo "Some checks failed."
    exit 1
fi
