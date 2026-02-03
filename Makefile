CC = gcc
WARNINGS = -Wall -Wextra
CFLAGS = -std=c99 -O2 $(shell pkg-config libevent_openssl --cflags --libs)
INCLUDES = -Isrc

all: server keys

server: src/server.c src/util.c src/secure.c src/packets.c
	$(CC) $(CFLAGS) $(WARNINGS) $(INCLUDES) \
		src/server.c src/util.c src/secure.c src/packets.c \
		-levent_openssl -lssl -lcrypto \
		-o server

keys:
	@if [ ! -f key.pem ] && [ ! -f cert.pem ]; then \
		openssl ecparam -name secp384r1 -genkey -noout -out key.pem && \
		openssl req -new -x509 -key key.pem -out cert.pem -days 365 \
			-subj "/CN=localhost" -sha384; \
	fi

clean:
	rm -f server

cleana:
	rm -f server cert.pem key.pem
