# Makefile для myRPC-server
# GNU Coding Standards совместимый

CC       = gcc
CFLAGS   = -Wall -Wextra -pedantic -std=c99 -g
CFLAGS  += -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
LDFLAGS  =

SRCDIR   = src
OBJDIR   = build
BINDIR   = ../dist

TARGET   = myrpc-server

SRCS     = $(SRCDIR)/main.c     \
           $(SRCDIR)/server.c   \
           $(SRCDIR)/config.c   \
           $(SRCDIR)/protocol.c \
           $(SRCDIR)/executor.c

OBJS     = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))

DEB_NAME    = myrpc-server
DEB_VERSION = 1.0.0
DEB_ARCH    = amd64
DEB_DIR     = $(OBJDIR)/deb/$(DEB_NAME)_$(DEB_VERSION)_$(DEB_ARCH)

# ----------------------------------------------------------------
# Цели
# ----------------------------------------------------------------

.PHONY: all clean deb install

all: $(BINDIR)/$(TARGET)

# Создание выходной директории
$(BINDIR)/$(TARGET): $(OBJS)
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo ">>> Built: $@"

# Компиляция объектных файлов
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Удаление всех временных файлов и бинарников
clean:
	rm -rf $(OBJDIR)
	rm -f $(BINDIR)/$(TARGET)
	@echo ">>> Cleaned"

# Сборка deb-пакета
deb: all
	@echo ">>> Building deb package $(DEB_NAME)_$(DEB_VERSION)"
	@mkdir -p $(DEB_DIR)/DEBIAN
	@mkdir -p $(DEB_DIR)/usr/bin
	@mkdir -p $(DEB_DIR)/etc/myRPC
	@mkdir -p $(DEB_DIR)/lib/systemd/system

	# Бинарник
	cp $(BINDIR)/$(TARGET) $(DEB_DIR)/usr/bin/

	# Конфигурационные файлы (примеры, не перезаписывают существующие)
	cp ../configs/myRPC.conf  $(DEB_DIR)/etc/myRPC/myRPC.conf
	cp ../configs/users.conf  $(DEB_DIR)/etc/myRPC/users.conf

	# systemd unit-файл
	cp ../configs/myrpc-server.service \
		$(DEB_DIR)/lib/systemd/system/

	# control
	printf "Package: $(DEB_NAME)\n\
Version: $(DEB_VERSION)\n\
Architecture: $(DEB_ARCH)\n\
Maintainer: Student <student@example.com>\n\
Description: myRPC server daemon\n\
 Remote bash command execution daemon.\n" \
		> $(DEB_DIR)/DEBIAN/control

	# conffiles — помечаем конфиги чтобы dpkg не перезаписывал
	printf "/etc/myRPC/myRPC.conf\n/etc/myRPC/users.conf\n" \
		> $(DEB_DIR)/DEBIAN/conffiles

	# postinst — включаем и запускаем службу
	printf "#!/bin/sh\nset -e\nsystemctl daemon-reload\n\
systemctl enable myrpc-server\nsystemctl start myrpc-server\n" \
		> $(DEB_DIR)/DEBIAN/postinst
	chmod 755 $(DEB_DIR)/DEBIAN/postinst

	# prerm — останавливаем службу
	printf "#!/bin/sh\nset -e\nsystemctl stop myrpc-server || true\n\
systemctl disable myrpc-server || true\n" \
		> $(DEB_DIR)/DEBIAN/prerm
	chmod 755 $(DEB_DIR)/DEBIAN/prerm

	dpkg-deb --build $(DEB_DIR)
	mv $(DEB_DIR).deb $(BINDIR)/
	@echo ">>> Package: $(BINDIR)/$(DEB_NAME)_$(DEB_VERSION)_$(DEB_ARCH).deb"

install: all
	install -m 755 $(BINDIR)/$(TARGET) /usr/bin/
	install -d /etc/myRPC
	install -m 644 ../configs/myRPC.conf /etc/myRPC/
	install -m 644 ../configs/users.conf /etc/myRPC/
