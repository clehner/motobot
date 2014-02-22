PREFIX ?= /usr/local
BINDIR  = $(PREFIX)/bin
BIN     = motobot
CC     ?= cc
SRC     = $(wildcard src/*.c)
SRC    += $(wildcard deps/*/*.c)
OBJ = $(SRC:.c=.o)
CFLAGS  = -std=c99 -Ideps -Isrc
CFLAGS += -Wall
LDFLAGS = -lircclient

$(BIN): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

.c.o:
	$(CC) -o $@ $< -c $(CFLAGS)

install: all
	install -m 0755 $(BIN) $(DESTDIR)$(BINDIR)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(BIN)

clean:
	rm -f $(BIN) $(OBJ)

.PHONY: clean
