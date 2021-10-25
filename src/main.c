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

#define LAB5E_HOST "192.168.1.16"
#define LAB5E_PORT 5683

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define FW_VERSION "1.0.0"
#define FW_MODEL "Model 1"
#define FW_SERIAL "00001"
#define FW_MANUFACTURER "Lab5e AS"

#define LAB5E_ENDPOINT = "coap://192.168.1.67:5683"

// This is the buffer we'll be using for messages.
#define BUF_SIZE 256
static uint8_t buffer[BUF_SIZE];

/*
 * @brief Report the firmware version to the Lab5e CoAP endpoint
 */
static int report_version(void) {
  // Encode a FOTA status report
  size_t sz = 0;
  // The TLV buffer is quite simple - it's just the series of fields.
  sz += encode_tlv_string(buffer + sz, FIRMWARE_VER_ID, FW_VERSION);
  sz += encode_tlv_string(buffer + sz, MODEL_NUMBER_ID, FW_MODEL);
  sz += encode_tlv_string(buffer + sz, SERIAL_NUMBER_ID, FW_SERIAL);
  sz += encode_tlv_string(buffer + sz, CLIENT_MANUFACTURER_ID, FW_MANUFACTURER);

  LOG_INF("Encoded FOTA buffer is %d bytes", sz);

  return coap_send_message(COAP_METHOD_POST, "fw", buffer, sz);
}

void main(void) {
  dhcp_init();
  int res = coap_start_client(LAB5E_HOST, LAB5E_PORT);
  LOG_INF("CoAP client started, result = %d", res);

  res = report_version();
  LOG_INF("FOTA report sent, result = %d", res);

  coap_stop_client();
}
