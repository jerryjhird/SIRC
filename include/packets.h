#ifndef PACKETS_H
#define PACKETS_H

#include <string.h>
#include <event2/bufferevent.h>

void irc_packet_nick(struct bufferevent *bev, const char *args);
void irc_packet_user(struct bufferevent *bev, const char *args);
void irc_packet_join(struct bufferevent *bev, const char *args);
void irc_packet_privmsg(struct bufferevent *bev, const char *args);
void irc_packet_unknown(struct bufferevent *bev, const char *line);


static inline void handle_line(struct bufferevent *bev, const char *line)
{
    if (strncmp(line, "NICK ", 5) == 0)
        irc_packet_nick(bev, line + 5);
    else if (strncmp(line, "USER ", 5) == 0)
        irc_packet_user(bev, line + 5);
    else if (strncmp(line, "JOIN ", 5) == 0)
        irc_packet_join(bev, line + 5);
    else if (strncmp(line, "PRIVMSG ", 8) == 0)
        irc_packet_privmsg(bev, line + 8);
    else
        irc_packet_unknown(bev, line);
}

#endif
