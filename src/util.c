#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "util.h"

#define MAIN_CHANNEL "#main"

struct client *clients = NULL;

void add_client(struct bufferevent *bev, const char *nick) {
    struct client *c = malloc(sizeof(struct client));
    if (!c) return;
    c->bev = bev;

    // nick
    strncpy(c->nick, nick ? nick : "guest", sizeof(c->nick) - 1);

    c->nick[sizeof(c->nick) - 1] = '\0';
    c->channel[0] = '\0';
    c->next = clients;
    clients = c;

}

void remove_client(struct bufferevent *bev) {
    struct client **p = &clients;
    while (*p) {
        if ((*p)->bev == bev) {
            struct client *to_free = *p;

            // announce part
            char buf[128];
            snprintf(buf, sizeof(buf), ":%s PART %s\r\n", to_free->nick, MAIN_CHANNEL);
            for (struct client *iter = clients; iter; iter = iter->next) {
                if (iter != to_free)
                    bufferevent_write(iter->bev, buf, strlen(buf));
            }

            *p = (*p)->next;
            bufferevent_free(to_free->bev);
            free(to_free);
            return;
        }
        p = &(*p)->next;
    }
}

void broadcast(struct bufferevent *sender, const char *nick, const char *msg) {
    char buf[1024];
    // use the sender's channel
    char *channel = "#main";
    for (struct client *c = clients; c; c = c->next) {
        if (c->bev == sender) {
            channel = c->channel;
            break;
        }
    }

    snprintf(buf, sizeof(buf), ":%s PRIVMSG %s :%s\r\n", nick, channel, msg);
    for (struct client *c = clients; c; c = c->next) {
        if (c->bev != sender)
            bufferevent_write(c->bev, buf, strlen(buf));
    }
}
