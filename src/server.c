#include <event2/event.h>
#include <event2/bufferevent_ssl.h> 
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include "server.h"
#include "secure.h"
#include "packets.h"

extern SSL_CTX *ssl_ctx;

void read_cb(struct bufferevent *bev, void *ctx) {
    (void)ctx;
    struct evbuffer *input = bufferevent_get_input(bev);
    char *line;
    size_t n;

    while ((line = evbuffer_readln(input, &n, EVBUFFER_EOL_CRLF)) != NULL) {
        handle_packet(bev, line);
        free(line);
    }
}

void event_cb(struct bufferevent *bev, short events, void *ctx) {
    (void)ctx;
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        remove_client(bev);
    }
}

void accept_cb(struct evconnlistener *listener, evutil_socket_t fd,
                      struct sockaddr *addr, int socklen, void *ctx) {
    (void)addr;
    (void)socklen;
    (void)ctx;

    struct event_base *base = evconnlistener_get_base(listener);
    struct bufferevent *bev = create_ssl_bev(base, fd, ssl_ctx);

    struct client *c = add_client(bev, "guest");
    if (!c) {
        bufferevent_free(bev);
        return;
    }

    bufferevent_setcb(bev, read_cb, NULL, event_cb, c);
    bufferevent_enable(bev, EV_READ | EV_WRITE);

    printf("client connected.\n");
    const char *welcome = "Welcome\n";
    bufferevent_write(bev, welcome, strlen(welcome));
}