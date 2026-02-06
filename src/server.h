#ifndef UTIL_H
#define UTIL_H

#include <event2/bufferevent.h>

struct client {
    struct bufferevent *bev;
    char *nick;
    char *channel;
    int registered;
    struct client *next;
    int echo_message_enabled;
    char *away_msg;
    time_t away_since;
};

extern struct client *clients;

struct client *add_client(struct bufferevent *bev, const char *nick);
void remove_client(struct bufferevent *bev);
void broadcast(struct client *sender, const char *nick, const char *msg);

void read_cb(struct bufferevent *bev, void *ctx);
void event_cb(struct bufferevent *bev, short events, void *ctx);

#endif