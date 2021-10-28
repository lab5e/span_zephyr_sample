#pragma once

/**
 * @brief This is nothing exceptional, just the plain UDP bits. This will send 10 UDP or DTLS
 *         messages to the host and port specfied
 */
int send_udp(const char *host, const int port);