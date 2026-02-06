CC = gcc
WARNINGS = -Wall -Wextra -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wformat=2 -Wundef
INCLUDES = -Isrc
CFLAGS = -std=c99 -D_POSIX_C_SOURCE=200809L -O2 $(shell pkg-config libevent_openssl --cflags --libs) $(WARNINGS) $(INCLUDES)
SOURCES := src/main.c src/server.c src/clients.c src/secure.c src/packets.c

all: server keys

server: $(SOURCES)
	$(CC) $(CFLAGS) \
		$(SOURCES) \
		-levent_openssl -lssl -lcrypto \
		-o server

keys:
	@if [ ! -f key.pem ] && [ ! -f cert.pem ]; then \
		openssl ecparam -name secp384r1 -genkey -noout -out key.pem && \
		openssl req -new -x509 -key key.pem -out cert.pem -days 365 \
			-subj "/" -sha384; \
	fi

clean:
	rm -f server

cleana:
	rm -f server cert.pem key.pem
