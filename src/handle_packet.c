#include "packets.h"
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <event2/bufferevent.h>

/* command table */
typedef void (*irc_handler_fn)(struct bufferevent *, const char *);

struct irc_command {
    const char      *name;
    irc_handler_fn   handler;
};

static const struct irc_command irc_commands[] = {
    { "NICK",    irc_packet_nick },
    { "USER",    irc_packet_user },
    { "JOIN",    irc_packet_join },
    { "PRIVMSG", irc_packet_privmsg },
    { "CAP",     irc_packet_cap },
    { "MODE",    irc_packet_mode },
    { "WHO",     irc_packet_who },
    { "QUIT",    irc_packet_quit },
    { NULL,      NULL }
};

void irc_dispatch(struct bufferevent *bev, const char *cmd, const char *args)
{
    for (const struct irc_command *c = irc_commands; c->name; c++) {
        if (strcasecmp(c->name, cmd) == 0) {
            c->handler(bev, args);
            return;
        }
    }
    irc_packet_unknown(bev, cmd);
}

void handle_packet(struct bufferevent *bev, const char *line)
{
    if (!line || !*line) return;

    char buf[512];
    strncpy(buf, line, sizeof(buf)-1);
    buf[sizeof(buf)-1] = '\0';

    char *cmd = buf;
    char *args = NULL;

    if (*cmd == ':') {
        cmd = strchr(cmd, ' ');
        if (!cmd) return;
        cmd++; // skip space
    }

    // find first space
    char *space = strchr(cmd, ' ');
    if (space) {
        *space = '\0';
        args = space + 1;
        char *end = args + strlen(args) - 1;
        while (end >= args && (*end == '\r' || *end == '\n')) {
            *end = '\0';
            end--;
        }
    }

    char *end = cmd + strlen(cmd) - 1;
    while (end >= cmd && (*end == '\r' || *end == '\n')) {
        *end = '\0';
        end--;
    }

    irc_dispatch(bev, cmd, args);
}

