#include <event2/bufferevent.h>
#include "packets.h"
#include "handle_packet.c"
#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#define GET_CLIENT(bev) \
    struct client *c = NULL; \
    bufferevent_getcb(bev, NULL, NULL, NULL, (void**)&c); \
    if (!c) return;

// packet handlers
void irc_packet_nick(struct bufferevent *bev, const char *args)
{
    GET_CLIENT(bev);

    if (!args) return;

    strncpy(c->nick, args, sizeof(c->nick) - 1);
    c->nick[sizeof(c->nick) - 1] = 0;
}

void irc_packet_user(struct bufferevent *bev, const char *args)
{
    (void)args;
    GET_CLIENT(bev);
    c->registered = 1;
}

void irc_packet_join(struct bufferevent *bev, const char *args)
{
    (void)args;

    GET_CLIENT(bev);
    const char *nick = *c->nick ? c->nick : "guest";

    free(c->channel);
    c->channel = strdup("#main");

    char *buf;
    asprintf(&buf, ":%s!%s@localhost JOIN %s\r\n",
             nick, nick, c->channel);

    for (struct client *cl = clients; cl; cl = cl->next)
        bufferevent_write(cl->bev, buf, strlen(buf));

    free(buf);
}

void irc_packet_privmsg(struct bufferevent *bev, const char *args)
{
    GET_CLIENT(bev);
    if (!args) return;

    const char *msg = strchr(args, ':');

    if (!msg) return;
    msg++;

    broadcast(c, (*c->nick) ? c->nick : "guest", msg);
}

void irc_packet_cap(struct bufferevent *bev, const char *args)
{
    GET_CLIENT(bev);

    // CAP LS
    if (args && strncmp(args, "LS", 2) == 0) {
        bufferevent_write(bev, "CAP * LS :echo-message\r\n", 23);
    } else if (args && strstr(args, "echo-message")) {
        c->echo_message_enabled = 1;
        bufferevent_write(bev, "CAP * ACK :echo-message\r\n", 25);
    }

    bufferevent_write(bev, "CAP END\r\n", 9);
}

void irc_packet_mode(struct bufferevent *bev, const char *args)
{
    GET_CLIENT(bev);
    if (!args) return;

    char *buf;
    asprintf(&buf, ":server 324 %s %s +\r\n", c->nick, args);
    bufferevent_write(bev, buf, strlen(buf));
    free(buf);
}

void irc_packet_who(struct bufferevent *bev, const char *args)
{
    GET_CLIENT(bev);

    char *buf;
    asprintf(&buf,
        ":server 315 %s %s :End of WHO list\r\n",
        c->nick, (args && *args) ? args : "*");

    bufferevent_write(bev, buf, strlen(buf));
    free(buf);
}

void irc_packet_list(struct bufferevent *bev, const char *args)
{
    GET_CLIENT(bev);
    (void)args;

    char *buf;
    asprintf(&buf, ":server 322 %s #main 1 :Main chat channel\r\n", c->nick);
    bufferevent_write(bev, buf, strlen(buf));
    free(buf);

    asprintf(&buf, ":server 323 %s :End of LIST\r\n", c->nick);
    bufferevent_write(bev, buf, strlen(buf));
    free(buf);
}

void irc_packet_ping(struct bufferevent *bev, const char *args)
{
    GET_CLIENT(bev);
    if (!args) return;

    char *buf;
    asprintf(&buf, "PONG :%s\r\n", args);
    bufferevent_write(bev, buf, strlen(buf));
    free(buf);
}

void irc_packet_part(struct bufferevent *bev, const char *args)
{
    GET_CLIENT(bev);
    if (!c->channel) return;

    const char *nick = (*c->nick) ? c->nick : "guest";
    const char *part_msg = (args && *args) ? args : "leaving";

    char *buf;
    asprintf(&buf, ":%s PART %s :%s\r\n", nick, c->channel, part_msg);

    for (struct client *cl = clients; cl; cl = cl->next)
        if (cl != c)
            bufferevent_write(cl->bev, buf, strlen(buf));

    free(buf);
    free(c->channel);
    c->channel = NULL;
}

void irc_packet_topic(struct bufferevent *bev, const char *args)
{
    GET_CLIENT(bev);
    if (!c->channel) return;

    const char *topic = (args && *args) ? args : "";
    char *buf;
    asprintf(&buf, ":server 332 %s %s :%s\r\n", c->nick, c->channel, topic);
    bufferevent_write(bev, buf, strlen(buf));
    free(buf);
}

void irc_packet_names(struct bufferevent *bev, const char *args)
{
    GET_CLIENT(bev);

    const char *channel = (args && *args) ? args : (c->channel ? c->channel : "#main");
    char *buf;
    asprintf(&buf, ":server 353 %s = %s :", c->nick, channel);
    
    for (struct client *cl = clients; cl; cl = cl->next) {
        if (cl->channel && strcmp(cl->channel, channel) == 0) {
            asprintf(&buf, "%s%s ", buf, (*cl->nick) ? cl->nick : "guest");
        }
    }

    size_t len = strlen(buf);
    buf[len - 1] = '\0';
    asprintf(&buf, "%s\r\n", buf);
    bufferevent_write(bev, buf, strlen(buf));
    free(buf);

    asprintf(&buf, ":server 366 %s %s :End of NAMES list\r\n", c->nick, channel);
    bufferevent_write(bev, buf, strlen(buf));
    free(buf);
}

void irc_packet_away(struct bufferevent *bev, const char *args)
{
    GET_CLIENT(bev);

    if (args && *args) {
        // set away message
        free(c->away_msg);
        c->away_msg = strdup(args);
        c->away_since = time(NULL);
        char buf[256];
        snprintf(buf, sizeof(buf), ":server 306 %s :You have been marked as away\r\n", c->nick);
        bufferevent_write(bev, buf, strlen(buf));
    } else {
        // remove away message
        free(c->away_msg);
        c->away_msg = NULL;
        c->away_since = 0;
        char buf[256];
        snprintf(buf, sizeof(buf), ":server 305 %s :You are no longer marked as away\r\n", c->nick);
        bufferevent_write(bev, buf, strlen(buf));
    }
}

void irc_packet_whois(struct bufferevent *bev, const char *args)
{
    GET_CLIENT(bev);
    if (!args || !*args) return;

    // find target client
    struct client *target = NULL;
    for (struct client *cl = clients; cl; cl = cl->next) {
        if (strcmp(cl->nick, args) == 0) {
            target = cl;
            break;
        }
    }

    if (!target) {
        // user not found
        char buf[256];
        snprintf(buf, sizeof(buf), ":server 401 %s %s :No such nick\r\n", c->nick, args);
        bufferevent_write(bev, buf, strlen(buf));
        return;
    }

    char buf[512];

    // 311 RPL_WHOISUSER
    snprintf(buf, sizeof(buf),
             ":server 311 %s %s %s %s\r\n",
             c->nick, target->nick, target->nick, SERVER_NAME);
    bufferevent_write(bev, buf, strlen(buf));

    // 301 RPL_AWAY
    if (target->away_msg) {
        snprintf(buf, sizeof(buf),
                 ":server 301 %s %s :%s\r\n",
                 c->nick, target->nick, target->away_msg);
        bufferevent_write(bev, buf, strlen(buf));
    }

    // 312 RPL_WHOISSERVER
    snprintf(buf, sizeof(buf),
             ":server 312 %s %s %s :JhirdIRC\r\n",
             c->nick, target->nick, SERVER_NAME);
    bufferevent_write(bev, buf, strlen(buf));

    // 317 RPL_WHOISIDLE
    time_t idle = 0;
    if (target->away_msg && target->away_since > 0)
        idle = time(NULL) - target->away_since;

    snprintf(buf, sizeof(buf),
             ":server 317 %s %s %ld :seconds idle\r\n",
             c->nick, target->nick, (long)idle);
    bufferevent_write(bev, buf, strlen(buf));

    // 318 RPL_ENDOFWHOIS
    snprintf(buf, sizeof(buf),
             ":server 318 %s %s :\r\n",
             c->nick, target->nick);
    bufferevent_write(bev, buf, strlen(buf));
}

void irc_packet_quit(struct bufferevent *bev, const char *args)
{
    GET_CLIENT(bev);

    const char *nick = *c->nick ? c->nick : "guest";
    const char *quit_msg = (args && *args) ? args : "leaving";

    char *buf;
    asprintf(&buf, ":%s QUIT :%s\r\n", nick, quit_msg);

    for (struct client *cl = clients; cl; cl = cl->next)
        if (cl != c)
            bufferevent_write(cl->bev, buf, strlen(buf));

    free(buf);
    remove_client(bev);
}

void irc_packet_unknown(struct bufferevent *bev, const char *line)
{
    (void)bev;
    (void)line;
}
