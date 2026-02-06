#ifndef PACKETS_H
#define PACKETS_H

#include <string.h>
#include <event2/bufferevent.h>

void irc_packet_nick(struct bufferevent *bev, const char *args);
void irc_packet_user(struct bufferevent *bev, const char *args);
void irc_packet_join(struct bufferevent *bev, const char *args);
void irc_packet_privmsg(struct bufferevent *bev, const char *args);
void irc_packet_cap(struct bufferevent *bev, const char *args);
void irc_packet_mode(struct bufferevent *bev, const char *args);
void irc_packet_who(struct bufferevent *bev, const char *args);
void irc_packet_list(struct bufferevent *bev, const char *args);
void irc_packet_ping(struct bufferevent *bev, const char *args);
void irc_packet_part(struct bufferevent *bev, const char *args);
void irc_packet_topic(struct bufferevent *bev, const char *args);
void irc_packet_names(struct bufferevent *bev, const char *args);
void irc_packet_away(struct bufferevent *bev, const char *args);
void irc_packet_whois(struct bufferevent *bev, const char *args);
void irc_packet_quit(struct bufferevent *bev, const char *args);
void irc_packet_unknown(struct bufferevent *bev, const char *line);

void prefix_command_reqecho(struct bufferevent *bev, const char *args);

void handle_packet(struct bufferevent *bev, const char *line);

#endif
