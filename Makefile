PREFIX ?= /usr/local
BINDIR  = $(PREFIX)/bin
CONFDIR = /etc/motobot
BIN     = motobot
CC     ?= cc
SRC     = $(wildcard src/*.c)
SRC    += $(wildcard deps/*/*.c)
OBJ = $(SRC:.c=.o)
CFLAGS  = -std=c99 -Ideps -Isrc
CFLAGS += -Wall
LDFLAGS = -lircclient

all: $(BIN) config.ini

$(BIN): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

.c.o:
	$(CC) -o $@ $< -c $(CFLAGS)

config.ini:
	cp -i config.def.ini $@

install: install-bin install-conf

install-bin: all
	install -m 0755 $(BIN) $(DESTDIR)$(BINDIR)

install-conf: config.ini
	install -m 0755 -d $(DESTDIR)$(CONFDIR)
	install -m 0644 config.ini $(DESTDIR)$(CONFDIR)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(BIN)
	rmdir $(DESTDIR)$(CONFDIR)/$(BIN)

clean:
	rm -f $(BIN) $(OBJ)

.PHONY: clean install install-bin install-conf uninstall
