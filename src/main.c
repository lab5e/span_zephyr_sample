#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <logging/log.h>
#include <net/coap.h>
#include <net/socket.h>
#include <zephyr.h>

#include "udp-client.h"
#include "coap-client.h"
#include "fota_report.h"
#include "networking.h"

#include "clientcert.h"

// This is the buffer we'll be using for messages.
#define BUF_SIZE 256
static uint8_t buffer[BUF_SIZE];

// Test host. The real host will be "data.lab5e.com:5684" for external clients
// and "172.16.15.14:5683" for internal (ie CIoT) clients.

#define LAB5E_HOST "192.168.1.118"
#define LAB5E_COAP_PORT 5684
#define LAB5E_UDP_PORT 1234
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define FW_VERSION "1.0.0"
#define FW_MODEL "Model 1"
#define FW_SERIAL "00001"
#define FW_MANUFACTURER "Lab5e AS"

/**
 * This is the callback for the blockwise transfer. It is called once for every
 * block returned from the server. The offset is the offset (in bytes) into the
 * file
 */
static int bw_callback(bool last, uint32_t offset, uint8_t *buffer,
                       size_t len)
{
  static int block_count;
  block_count++;
  if (last)
  {
    LOG_INF("Received last block (of %d blocks) with %d bytes totalt",
            block_count, offset + len);
    block_count = 0;
  }
  return 0;
}

/*
 * @brief Report the firmware version to the Lab5e CoAP endpoint
 */
static int report_version(void)
{
  fota_report_t report = {.manufacturer = FW_MANUFACTURER,
                          .model = FW_MODEL,
                          .serial = FW_SERIAL,
                          .version = FW_VERSION};

  size_t sz = encode_fota_report(&report, buffer);

  int ret = coap_send_message(COAP_METHOD_POST, "u", buffer, sz);
  if (ret < 0)
  {
    LOG_ERR("Error sending message: %d", ret);
    return ret;
  }

  uint8_t code = 0;
  ret = coap_read_message(&code, buffer, &sz);
  if (ret < 0)
  {
    LOG_ERR("Error receving message: %d", ret);
    return ret;
  }

  fota_response_t resp;

  ret = decode_fota_response(&resp, buffer, sz);
  if (ret == 0)
  {
    LOG_INF("Host: %s", log_strdup(resp.host));
    LOG_INF("Port: %d", resp.port);
    LOG_INF("Path: %s", log_strdup(resp.path));
    LOG_INF("Available: %d", resp.update);
  }

  coap_blockwise_transfer("fw", bw_callback);
  return 0;
}

void main(void)
{
  dhcp_init();
  int res = coap_start_client(LAB5E_HOST, LAB5E_COAP_PORT);

  res = report_version();

  for (int i = 0; i < 10; i++)
  {
    buffer[0] = (uint8_t)i;
    res = coap_send_message(COAP_METHOD_POST, "data/on/server", buffer, 1);
    if (res >= 0)
    {
      size_t len = 0;
      uint8_t code = 0;
      res = coap_read_message(&code, buffer, &len);
      if (res >= 0)
      {
        LOG_INF("Message %d sent successfully, code=%d, %d bytes returned from "
                "server",
                i, code, len);
      }
    }
    else
    {
      goto ohnoes;
    }
    k_sleep(K_MSEC(250));
  }
ohnoes:
  coap_stop_client();

  send_udp(LAB5E_HOST, LAB5E_UDP_PORT);
}
