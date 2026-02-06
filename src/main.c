#include <event2/event.h>
#include <event2/bufferevent_ssl.h> 
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include "stdio.h"
#include <sys/inotify.h>
#include <unistd.h>
#include "secure.h"

SSL_CTX *ssl_ctx = NULL;

void accept_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int socklen, void *ctx);

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s <cert.pem> <key.pem>\n", argv[0]);
        return 1;
    }

    const char *cert_path = argv[1];
    const char *key_path  = argv[2];

    struct event_base *base = event_base_new();
    if (!base) {
        fprintf(stderr, "Could not init libevent!\n");
        return 1;
    }

    ssl_ctx = init_ssl_ctx(cert_path, key_path);
    if (!ssl_ctx) {
        fprintf(stderr, "Failed to init SSL context.\n");
        event_base_free(base);
        return 1;
    }

    struct sockaddr_in sin = {0};
    sin.sin_family = AF_INET;
    sin.sin_port = htons(1816);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);

    struct evconnlistener *listener =
        evconnlistener_new_bind(
            base,
            accept_cb,
            NULL,
            LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
            -1,
            (struct sockaddr*)&sin,
            sizeof(sin)
        );

    if (!listener) {
        perror("couldnt create listener");
        cleanup_ssl_ctx(ssl_ctx);
        event_base_free(base);
        return 1;
    }

    printf("server running on port %d...\n", 1816);
    event_base_dispatch(base);

    evconnlistener_free(listener);
    cleanup_ssl_ctx(ssl_ctx);
    event_base_free(base);
    return 0;
}
