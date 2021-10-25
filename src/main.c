#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <logging/log.h>
#include <net/coap.h>
#include <net/socket.h>
#include <zephyr.h>

#include "coap-client.h"
#include "fota_report.h"
#include "networking.h"

// This is the buffer we'll be using for messages.
#define BUF_SIZE 256
static uint8_t buffer[BUF_SIZE];

// Test host. The real host will be "data.lab5e.com:5684" for external clients
// and "172.16.15.14:5683" for internal (ie CIoT) clients.

#define LAB5E_HOST "192.168.1.67"
#define LAB5E_PORT 5683

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define FW_VERSION "1.0.0"
#define FW_MODEL "Model 1"
#define FW_SERIAL "00001"
#define FW_MANUFACTURER "Lab5e AS"

/*
 * @brief Report the firmware version to the Lab5e CoAP endpoint
 */
static int report_version(void) {
  fota_report_t report = {.manufacturer = FW_MANUFACTURER,
                          .model = FW_MODEL,
                          .serial = FW_SERIAL,
                          .version = FW_VERSION};

  size_t sz = encode_fota_report(&report, buffer);
  LOG_INF("Encoded FOTA buffer is %d bytes", sz);

  int ret = coap_send_message(COAP_METHOD_POST, "u", buffer, sz);
  if (ret < 0) {
    LOG_ERR("Error sending message: %d", ret);
    return ret;
  }

  uint8_t code = 0;
  ret = coap_read_message(&code, buffer, &sz);
  if (ret < 0) {
    LOG_ERR("Error receving message: %d", ret);
    return ret;
  }

  LOG_INF("Got %d bytes back from the server", sz);
  fota_response_t resp;

  ret = decode_fota_response(&resp, buffer, sz);
  if (ret == 0) {
    LOG_INF("Host: %s", log_strdup(resp.host));
    LOG_INF("Port: %d", resp.port);
    LOG_INF("Path: %s", log_strdup(resp.path));
    LOG_INF("Available: %d", resp.update);
  }
  return 0;
}

void main(void) {
  dhcp_init();
  int res = coap_start_client(LAB5E_HOST, LAB5E_PORT);
  LOG_INF("CoAP client started, result = %d", res);

  res = report_version();
  LOG_INF("FOTA report sent, result = %d", res);

  // Download image w/ callback

  // Push demo data

  coap_stop_client();
  k_sleep(K_FOREVER);
}
