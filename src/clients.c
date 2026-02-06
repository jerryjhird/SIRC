#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "server.h"
#include "string.h"
#include <event2/event.h>

#define MAIN_CHANNEL "#main"

struct client *clients = NULL;

struct client *add_client(struct bufferevent *bev, const char *nick) {
    struct client *c = malloc(sizeof(struct client));
    if (!c) return NULL;

    c->bev = bev;
    c->nick = strdup(nick ? nick : "guest");
    c->channel = strdup(MAIN_CHANNEL);

    c->next = clients;
    clients = c;

    c->away_msg = NULL;

    bufferevent_setcb(bev, read_cb, NULL, event_cb, c);
    bufferevent_enable(bev, EV_READ | EV_WRITE);

    return c;
}

void remove_client(struct bufferevent *bev) {
    struct client *to_free = NULL;

    for (struct client **p = &clients; *p; p = &(*p)->next) {
        if ((*p)->bev == bev) {
            to_free = *p;
            *p = (*p)->next;
            break;
        }
    }

    if (!to_free) return;

    size_t len = snprintf(NULL, 0, ":%s PART %s\r\n", to_free->nick, MAIN_CHANNEL);
    char *buf = malloc(len + 1);
    if (buf) {
        snprintf(buf, len + 1, ":%s PART %s\r\n", to_free->nick, MAIN_CHANNEL);
        for (struct client *c = clients; c; c = c->next) {
            if (c != to_free)
                bufferevent_write(c->bev, buf, len);
        }
        free(buf);
    }

    bufferevent_free(to_free->bev);
    free(to_free->nick);
    free(to_free->channel);
    free(to_free);
}

void broadcast(struct client *sender, const char *nick, const char *msg) {
    if (!sender || !nick || !msg) return;

    size_t len = snprintf(NULL, 0, ":%s PRIVMSG %s :%s\r\n", nick, sender->channel, msg);
    char *buf = malloc(len + 1);
    if (!buf) return;
    snprintf(buf, len + 1, ":%s PRIVMSG %s :%s\r\n", nick, sender->channel, msg);

    for (struct client *c = clients; c; c = c->next) {
        if (c == sender && !c->echo_message_enabled)
            continue;

        bufferevent_write(c->bev, buf, len);
    }

    free(buf);
}