#ifndef SERVER_NET_HPP
#define SERVER_NET_HPP

#include <stdint.h>

int net_init(const char *addr, uint16_t port);
int net_event_loop();
int net_deinit();

#endif
