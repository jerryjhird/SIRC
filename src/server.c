#include <event2/event.h>
#include <event2/bufferevent_ssl.h> 
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include "util.h"
#include "conf.h"
#include "secure.h"
#include "packets.h"

static SSL_CTX *ssl_ctx = NULL;

static void read_cb(struct bufferevent *bev, void *ctx) {
    (void)ctx;
    char buf[1024];
    int n = bufferevent_read(bev, buf, sizeof(buf) - 1);
    buf[n] = '\0';
    printf("Got: %s", buf);

    // handle each line separately
    char *line = strtok(buf, "\r\n");
    while (line) {
        handle_line(bev, line);
        line = strtok(NULL, "\r\n");
    }
}

static void event_cb(struct bufferevent *bev, short events, void *ctx) {
    (void)ctx;
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        printf("client disconnected.\n");
        remove_client(bev);
    }
}

static void accept_cb(struct evconnlistener *listener, evutil_socket_t fd,
                      struct sockaddr *addr, int socklen, void *ctx) {
    (void)addr;
    (void)socklen;
    (void)ctx;

    struct event_base *base = evconnlistener_get_base(listener);
    struct bufferevent *bev = create_ssl_bev(base, fd, ssl_ctx);

    // add client with temporary nick "guest"
    add_client(bev, "guest");

    bufferevent_setcb(bev, read_cb, NULL, event_cb, NULL);
    bufferevent_enable(bev, EV_READ | EV_WRITE);

    printf("TLS client connected.\n");
    const char *welcome = "Welcome\n";
    bufferevent_write(bev, welcome, strlen(welcome));
}

int main(void) {
    struct event_base *base = event_base_new();
    if (!base) {
        fprintf(stderr, "Could not init libevent!\n");
        return 1;
    }

    ssl_ctx = init_ssl_ctx("cert.pem", "key.pem");
    if (!ssl_ctx) {
        fprintf(stderr, "Failed to init SSL context.\n");
        return 1;
    }

    struct sockaddr_in sin = {0};
    sin.sin_family = AF_INET;
    sin.sin_port = htons(CONF_PORT); // standard IRC SSL port
    sin.sin_addr.s_addr = htonl(INADDR_ANY);

    struct evconnlistener *listener =
        evconnlistener_new_bind(base, accept_cb, NULL,
                                LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1,
                                (struct sockaddr*)&sin, sizeof(sin));

    if (!listener) {
        perror("couldnt create listener");
        cleanup_ssl_ctx(ssl_ctx);
        event_base_free(base);
        return 1;
    }

    printf("server running on port %d...\n", CONF_PORT);
    event_base_dispatch(base);

    evconnlistener_free(listener);
    cleanup_ssl_ctx(ssl_ctx);
    event_base_free(base);
    return 0;
}
