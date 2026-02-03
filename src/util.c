#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "util.h"

#define MAIN_CHANNEL "#main"

struct client *clients = NULL;

char *my_strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *p = malloc(len + 1);
    if (!p) return NULL;
    memcpy(p, s, len + 1);
    return p;
}

void add_client(struct bufferevent *bev, const char *nick) {
    struct client *c = malloc(sizeof(struct client));
    if (!c) return;
    c->bev = bev;

    c->nick = my_strdup(nick ? nick : "guest");
    c->channel = my_strdup(MAIN_CHANNEL);
    c->next = clients;
    clients = c;
}

void remove_client(struct bufferevent *bev) {
    struct client **p = &clients;
    while (*p) {
        if ((*p)->bev == bev) {
            struct client *to_free = *p;

            const char *prefix = ":";
            const char *middle = " PART ";
            const char *channel = MAIN_CHANNEL;
            const char *suffix = "\r\n";

            size_t len = 0;
            len += strlen(prefix);
            len += strlen(to_free->nick);
            len += strlen(middle);
            len += strlen(channel);
            len += strlen(suffix);

            char *buf = malloc(len + 1);
            if (buf) {
                size_t pos = 0;
                const char *pieces[] = {prefix, to_free->nick, middle, channel, suffix};
                size_t pieces_count = sizeof(pieces) / sizeof(pieces[0]);
                for (size_t i = 0; i < pieces_count; i++) {
                    const char *s = pieces[i];
                    for (size_t j = 0; s[j]; j++)
                        buf[pos++] = s[j];
                }
                buf[pos] = '\0';

                for (struct client *iter = clients; iter; iter = iter->next) {
                    if (iter != to_free)
                        bufferevent_write(iter->bev, buf, pos);
                }

                free(buf);
            }

            *p = (*p)->next;

            bufferevent_free(to_free->bev);
            free(to_free->nick);
            free(to_free->channel);
            free(to_free);
            return;
        }
        p = &(*p)->next;
    }
}

void broadcast(struct bufferevent *sender, const char *nick, const char *msg) {
    char *channel = "#main";
    for (struct client *c = clients; c; c = c->next) {
        if (c->bev == sender) {
            channel = c->channel;
            break;
        }
    }

    size_t len = snprintf(NULL, 0, ":%s PRIVMSG %s :%s\r\n", nick, channel, msg);
    char *buf = malloc(len + 1);
    if (!buf) return;
    snprintf(buf, len + 1, ":%s PRIVMSG %s :%s\r\n", nick, channel, msg);

    for (struct client *c = clients; c; c = c->next) {
        if (c->bev != sender)
            bufferevent_write(c->bev, buf, strlen(buf));
    }

    free(buf);
}
