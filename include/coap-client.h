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
 * @param code response code from server
 * @param buffer buffer with data from server
 * @param len length of buffer
 * @return Number of bytes received
 */
int coap_read_message(uint8_t *code, uint8_t *buffer, size_t *len);

/**
 * @brief callback for blockwise transfers.
 * @param last set to true when this is the last block
 * @param offset byte offset
 * @param buffer data buffer
 * @param len length of buffer
 * @return 0 if ok, any other value to stop blockwise transfer
 */
typedef int (*blockwise_callback_t)(bool last, uint32_t offset, uint8_t *buffer, size_t len);

/**
 * @brief Use blockwise transfers (with a GET request). This function returns when the download has
 *        completed.
 * @param path path to resource
 * @param callback callback function for data blocks
 */
int coap_blockwise_transfer(const char *path, blockwise_callback_t callback);