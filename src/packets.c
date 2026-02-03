#include <event2/bufferevent.h>
#include "packets.h"
#include "handle_packet.c"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// helpers
static struct client *find_client(struct bufferevent *bev)
{
    for (struct client *c = clients; c; c = c->next)
        if (c->bev == bev)
            return c;
    return NULL;
}

// packet handlers
void irc_packet_nick(struct bufferevent *bev, const char *args)
{
    struct client *c = find_client(bev);
    if (c && args) strncpy(c->nick, args, sizeof(c->nick)-1), c->nick[sizeof(c->nick)-1]=0;
}

void irc_packet_user(struct bufferevent *bev, const char *args)
{
    (void)args;
    struct client *c = find_client(bev);
    if (c) c->registered = 1;
}

void irc_packet_join(struct bufferevent *bev, const char *args)
{
    (void)args;
    struct client *c = find_client(bev);
    if (!c) return;
    const char *nick = (*c->nick) ? c->nick : "guest";
    free(c->channel); c->channel = strdup("#main");
    char *buf; asprintf(&buf, ":%s!%s@localhost JOIN %s\r\n", nick, nick, c->channel);
    for (struct client *cl = clients; cl; cl = cl->next) bufferevent_write(cl->bev, buf, strlen(buf));
    free(buf);
}

void irc_packet_privmsg(struct bufferevent *bev, const char *args)
{
    struct client *c = find_client(bev);
    if (!c || !args) return;
    const char *msg = strchr(args, ':'); if (!msg) return; msg++;
    broadcast(bev, (*c->nick)?c->nick:"guest", msg);
}

void irc_packet_cap(struct bufferevent *bev, const char *args)
{
    (void)args;
    bufferevent_write(bev, "CAP END\r\n", 9);
}

void irc_packet_mode(struct bufferevent *bev, const char *args)
{
    struct client *c = find_client(bev);
    if (!c || !args) return;
    char *buf; asprintf(&buf, ":server 324 %s %s +\r\n", c->nick, args);
    bufferevent_write(bev, buf, strlen(buf));
    free(buf);
}

void irc_packet_who(struct bufferevent *bev, const char *args)
{
    struct client *c = find_client(bev);
    if (!c) return;
    char *buf; asprintf(&buf, ":server 315 %s %s :End of WHO list\r\n", c->nick, (args&&*args)?args:"*");
    bufferevent_write(bev, buf, strlen(buf));
    free(buf);
}

void irc_packet_quit(struct bufferevent *bev, const char *args)
{
    struct client *c = find_client(bev);
    if (!c) return;

    const char *nick = (c->nick && *c->nick) ? c->nick : "guest";
    const char *quit_msg = (args && *args) ? args : "leaving";

    char *buf;
    asprintf(&buf, ":%s QUIT :%s\r\n", nick, quit_msg);
    if (buf) {
        for (struct client *cl = clients; cl; cl = cl->next) {
            if (cl != c)
                bufferevent_write(cl->bev, buf, strlen(buf));
        }
        free(buf);
    }

    printf("%s left\n", nick);
    remove_client(bev);
}

void irc_packet_unknown(struct bufferevent *bev, const char *line)
{
    (void)bev;
    printf("unknown command: %s\n", line);
}
