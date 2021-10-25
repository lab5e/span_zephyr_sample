#pragma once
#include <zephyr.h>

#include <sys/types.h>

/**
 * @brief Start the CoAP client
 */
int coap_start_client(const char *host, uint16_t port);

/**
 * @brief Stop the CoAP client
 */
int coap_stop_client(void);

/**
 * @brief Send message via the CoAP client.
 * @param method CoAP method (COAP_METHOD_GET, COAP_METHOD_POST) to use
 * @param path The path to use when sending the request
 * @param buffer The buffer to send
 * @param len The length of the buffer
 */
int coap_send_message(const uint8_t method, const char *path, const uint8_t *buffer, size_t len);


/**
 * @brief Read message from CoAP client. The function will not block
 * @return Number of bytes received
 */
int coap_read_message(const uint8_t *buffer, size_t *len);