# Общий Makefile myRPC — собирает клиент и сервер
# Использование:
#   make all    — собрать всё
#   make clean  — удалить артефакты сборки
#   make deb    — собрать deb-пакеты

.PHONY: all clean deb

all:
	@echo "=== Building myRPC-client ==="
	$(MAKE) -C client all
	@echo "=== Building myRPC-server ==="
	$(MAKE) -C server all
	@echo "=== Build complete. Binaries in dist/ ==="

clean:
	@echo "=== Cleaning myRPC-client ==="
	$(MAKE) -C client clean
	@echo "=== Cleaning myRPC-server ==="
	$(MAKE) -C server clean
	@rm -rf dist/
	@echo "=== Clean complete ==="

deb: all
	@echo "=== Building deb packages ==="
	$(MAKE) -C client deb
	$(MAKE) -C server deb
	@echo "=== deb packages in dist/ ==="
	@ls dist/*.deb 2>/dev/null || echo "(no .deb files found)"
