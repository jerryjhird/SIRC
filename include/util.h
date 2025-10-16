#ifndef UTIL_H
#define UTIL_H

#include <event2/bufferevent.h>

struct client {
    struct bufferevent *bev;
    char nick[32];
    char channel[32];
    int registered;    // track USER/NICK registration per client
    struct client *next;
};

extern struct client *clients;

void add_client(struct bufferevent *bev, const char *nick);
void remove_client(struct bufferevent *bev);
void broadcast(struct bufferevent *sender, const char *nick, const char *msg);

#endif