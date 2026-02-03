#include "secure.h"
#include <stdio.h>
#include <openssl/err.h>
#include <openssl/evp.h>

SSL_CTX *init_ssl_ctx(const char *cert_file, const char *key_file) {
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        fprintf(stderr, "Unable to create SSL context\n");
        return NULL;
    }

    // TLS 1.3 only
    SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION);
    SSL_CTX_set_max_proto_version(ctx, TLS1_3_VERSION);

    if (SSL_CTX_use_certificate_file(ctx, cert_file, SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return NULL;
    }

    SSL_CTX_set_options(ctx, SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_TLSv1_2);

    return ctx;
}


struct bufferevent *create_ssl_bev(struct event_base *base, evutil_socket_t fd, SSL_CTX *ctx) {
    SSL *ssl = SSL_new(ctx);
    return bufferevent_openssl_socket_new(base, fd, ssl,
                                          BUFFEREVENT_SSL_ACCEPTING,
                                          BEV_OPT_CLOSE_ON_FREE);
}


void cleanup_ssl_ctx(SSL_CTX *ctx) {
    if (ctx) SSL_CTX_free(ctx);
    EVP_cleanup();
}
