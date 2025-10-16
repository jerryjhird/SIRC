#ifndef SEC_H
#define SEC_H

#include <openssl/ssl.h>
#include <event2/event.h>
#include <event2/bufferevent_ssl.h>

SSL_CTX *init_ssl_ctx(const char *cert_file, const char *key_file);
struct bufferevent *create_ssl_bev(struct event_base *base, evutil_socket_t fd, SSL_CTX *ctx);
void cleanup_ssl_ctx(SSL_CTX *ctx);

#endif