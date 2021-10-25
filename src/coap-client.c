#include <zephyr.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(coap_client, LOG_LEVEL_DBG);

#include <net/socket.h>

#include <net/coap.h>
#include <net/net_ip.h>
#include <net/udp.h>

#include <errno.h>

#include "coap-client.h"

/* CoAP socket fd */
static int sock;

struct pollfd fds[1];
static int nfds;

static void prepare_fds(void) {
  fds[nfds].fd = sock;
  fds[nfds].events = POLLIN;
  nfds++;
}

int coap_start_client(const char *host, uint16_t port) {
  int ret = 0;
  struct sockaddr_in addr;

  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  inet_pton(AF_INET, host, &addr.sin_addr);

  sock = socket(addr.sin_family, SOCK_DGRAM, IPPROTO_UDP);
  if (sock < 0) {
    LOG_ERR("Failed to create UDP socket %d", errno);
    return -errno;
  }

  ret = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
  if (ret < 0) {
    LOG_ERR("Cannot connect to UDP remote : %d", errno);
    return -errno;
  }

  prepare_fds();

  return 0;
}

int coap_stop_client(void) {
  close(sock);
  return 0;
}

#define MAX_COAP_MSG_LEN 256
static uint8_t coap_data_buffer[MAX_COAP_MSG_LEN];

int coap_send_message(const uint8_t method, const char *path,
                      const uint8_t *buffer, size_t len) {
  struct coap_packet request;
  int r;

  memset(coap_data_buffer, 0, MAX_COAP_MSG_LEN);

  r = coap_packet_init(&request, coap_data_buffer, MAX_COAP_MSG_LEN,
                       COAP_VERSION_1, COAP_TYPE_CON, COAP_TOKEN_MAX_LEN,
                       coap_next_token(), method, coap_next_id());
  if (r < 0) {
    LOG_ERR("Failed to init CoAP message");
    return -ENOMEM;
  }

  r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH, path,
                                strlen(path));
  if (r < 0) {
    LOG_ERR("Unable add option to request");
    return -ENOMEM;
  }

  switch (method) {
  case COAP_METHOD_POST:
    r = coap_packet_append_payload_marker(&request);
    if (r < 0) {
      LOG_ERR("Unable to append payload marker");
      return -ENOMEM;
    }

    r = coap_packet_append_payload(&request, (uint8_t *)buffer,
                                   sizeof(buffer) - 1);
    if (r < 0) {
      LOG_ERR("Not able to append payload");
      return -ENOMEM;
    }

    break;
  case COAP_METHOD_GET:
    // Nothing left to do
    break;
  default:
    // Can't handle other methods
    return -EINVAL;
  }

  LOG_INF("Sending message (%d bytes)", request.offset);
  return send(sock, request.data, request.offset, 0);
}
