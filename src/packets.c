#include "packets.h"
#include "util.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

static struct client *find_client(struct bufferevent *bev)
{
    for (struct client *c = clients; c; c = c->next)
        if (c->bev == bev)
            return c;
    return NULL;
}


void irc_packet_nick(struct bufferevent *bev, const char *args)
{
    struct client *c = find_client(bev);
    if (!c) return;

    strncpy(c->nick, args, sizeof(c->nick) - 1);
    c->nick[sizeof(c->nick) - 1] = '\0';
}


void irc_packet_user(struct bufferevent *bev, const char *args)
{
    struct client *c = clients;
    while (c) {
        if (c->bev == bev)
            break;
        c = c->next;
    }
    if (!c) return;

    c->registered = 1;
}


void irc_packet_join(struct bufferevent *bev, const char *args)
{
    struct client *c = find_client(bev);
    if (!c) return;

    const char *nick = (*c->nick) ? c->nick : "guest";
    strncpy(c->channel, "#main", sizeof(c->channel) - 1);
    c->channel[sizeof(c->channel) - 1] = '\0';

    char buf[512];

    // announce join to all clients (todo: optimise or remove?)
    snprintf(buf, sizeof(buf), ":%s!%s@localhost JOIN #main\r\n", nick, nick);
    for (struct client *cl = clients; cl; cl = cl->next)
        bufferevent_write(cl->bev, buf, strlen(buf));

    snprintf(buf, sizeof(buf), ":server 332 %s #main :welcome to main\r\n", nick);
    bufferevent_write(bev, buf, strlen(buf));

    snprintf(buf, sizeof(buf), ":server 333 %s #main server %ld\r\n", nick, time(NULL));
    bufferevent_write(bev, buf, strlen(buf));

    bufferevent_write(bev, ":server 353 ", 12);
    bufferevent_write(bev, nick, strlen(nick));
    bufferevent_write(bev, " = #main :", 9);

    for (struct client *cl = clients; cl; cl = cl->next) {
        if (strcmp(cl->channel, "#main") == 0) {
            bufferevent_write(bev, " ", 1);
            bufferevent_write(bev, cl->nick, strlen(cl->nick));
        }
    }

    bufferevent_write(bev, "\r\n", 2);

    snprintf(buf, sizeof(buf), ":server 366 %s #main :End of /NAMES list\r\n", nick);
    bufferevent_write(bev, buf, strlen(buf));
}


void irc_packet_privmsg(struct bufferevent *bev, const char *args)
{
    struct client *c = find_client(bev);
    if (!c) return;

    const char *msg = strchr(args, ':');
    if (!msg) return;
    msg++;

    const char *nick = (*c->nick) ? c->nick : "guest";
    broadcast(bev, nick, msg);
}


void irc_packet_unknown(struct bufferevent *bev, const char *line)
{
    (void)bev; // unused for now
    printf("Unknown command: %s\n", line);
}
