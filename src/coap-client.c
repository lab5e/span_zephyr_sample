#include <errno.h>

#include <logging/log.h>
#include <zephyr.h>

#include <net/coap.h>
#include <net/net_ip.h>
#include <net/socket.h>
#include <net/udp.h>

LOG_MODULE_REGISTER(coap_client, LOG_LEVEL_DBG);

#include "coap-client.h"

#define BLOCK_WISE_TRANSFER_SIZE_GET 256
#define MAX_COAP_MSG_LEN (BLOCK_WISE_TRANSFER_SIZE_GET + 64)
static uint8_t coap_data_buffer[MAX_COAP_MSG_LEN];

/* CoAP socket fd */
static int sock;

struct pollfd fds[1];
static int nfds;

static void prepare_fds(void)
{
  fds[nfds].fd = sock;
  fds[nfds].events = POLLIN;
  nfds++;
}

int coap_start_client(const char *host, uint16_t port)
{
  int ret = 0;
  struct sockaddr_in addr;

  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  inet_pton(AF_INET, host, &addr.sin_addr);

  sock = socket(addr.sin_family, SOCK_DGRAM, IPPROTO_UDP);
  if (sock < 0)
  {
    LOG_ERR("Failed to create UDP socket %d", errno);
    return -errno;
  }

  ret = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
  if (ret < 0)
  {
    LOG_ERR("Cannot connect to UDP remote : %d", errno);
    return -errno;
  }

  prepare_fds();

  return 0;
}

int coap_stop_client(void)
{
  close(sock);
  return 0;
}

static int append_paths(struct coap_packet *request, const char *path)
{
  char tmp[32];
  char *p = (char *)path;
  char *start = p;
  int r;
  size_t len;
  while (*p)
  {
    if (*p == '/')
    {
      len = (p - start);
      memset(tmp, 0, 32);
      strncpy(tmp, start, len);
      r = coap_packet_append_option(request, COAP_OPTION_URI_PATH, tmp,
                                    len);
      if (r < 0)
      {
        LOG_ERR("Unable add option to request: %d", r);
        return -ENOMEM;
      }
      start = p + 1;
    }
    p++;
  }
  if (start < p)
  {
    r = coap_packet_append_option(request, COAP_OPTION_URI_PATH, start,
                                  strlen(start));
    if (r < 0)
    {
      LOG_ERR("Unable add option to request: %d", r);
      return -ENOMEM;
    }
  }
  return 0;
}

int coap_send_message(const uint8_t method, const char *path,
                      const uint8_t *buffer, size_t len)
{
  struct coap_packet request;
  int r;

  memset(coap_data_buffer, 0, MAX_COAP_MSG_LEN);

  r = coap_packet_init(&request, coap_data_buffer, MAX_COAP_MSG_LEN,
                       COAP_VERSION_1, COAP_TYPE_CON, COAP_TOKEN_MAX_LEN,
                       coap_next_token(), method, coap_next_id());
  if (r < 0)
  {
    LOG_ERR("Failed to init CoAP message: %d", r);
    return -ENOMEM;
  }

  r = append_paths(&request, path);
  if (r < 0)
  {
    return -ENOMEM;
  }

  switch (method)
  {
  case COAP_METHOD_POST:
    r = coap_packet_append_payload_marker(&request);
    if (r < 0)
    {
      LOG_ERR("Unable to append payload marker: %d", r);
      return -ENOMEM;
    }

    r = coap_packet_append_payload(&request, (uint8_t *)buffer, len);
    if (r < 0)
    {
      LOG_ERR("Not able to append payload: %d", r);
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

static void wait_for_data(void)
{
  if (poll(fds, nfds, -1) < 0)
  {
    LOG_ERR("Error in poll:%d", errno);
  }
}

int coap_read_message(uint8_t *code, uint8_t *buffer, size_t *len)
{
  memset(coap_data_buffer, 0, MAX_COAP_MSG_LEN);
  LOG_DBG("Wait for data");
  wait_for_data();
  LOG_DBG("Data waiting");

  int rcvd = recv(sock, coap_data_buffer, MAX_COAP_MSG_LEN, MSG_DONTWAIT);
  if (rcvd == 0)
  {
    *len = 0;
    LOG_DBG("No data in");
    return -EIO;
  }

  if (rcvd < 0)
  {
    LOG_ERR("Error reading data: %d", errno);
    if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
      *len = 0;
      return 0;
    }
    return -errno;
  }
  struct coap_packet reply;
  int ret = coap_packet_parse(&reply, coap_data_buffer, rcvd, NULL, 0);
  if (ret < 0)
  {
    LOG_ERR("Invalid CoAP packet returned: %d", ret);
    *len = 0;
    return 0;
  }
  *len = rcvd - reply.offset;
  memcpy(buffer, reply.data + reply.offset, *len);
  return *len;
}

int coap_blockwise_transfer(const char *path, blockwise_callback_t callback)
{
  if (!callback)
  {
    LOG_ERR("Can't do request to %s. Callback function is null", log_strdup(path));
    return -ENODATA;
  }
  struct coap_packet request;
  struct coap_packet reply;
  int r;
  struct coap_block_context blk_ctx;

  coap_block_transfer_init(&blk_ctx, COAP_BLOCK_256,
                           BLOCK_WISE_TRANSFER_SIZE_GET);

  bool last_block = false;
  size_t total_size = 0;
  while (!last_block)
  {
    memset(coap_data_buffer, 0, MAX_COAP_MSG_LEN);

    r = coap_packet_init(&request, coap_data_buffer, MAX_COAP_MSG_LEN,
                         COAP_VERSION_1, COAP_TYPE_CON, COAP_TOKEN_MAX_LEN,
                         coap_next_token(), COAP_METHOD_GET, coap_next_id());
    if (r < 0)
    {
      LOG_ERR("Failed to init CoAP message: %d", r);
      return -ENOMEM;
    }

    r = append_paths(&request, path);
    if (r < 0)
    {
      return -ENOMEM;
    }

    r = coap_append_block2_option(&request, &blk_ctx);
    if (r < 0)
    {
      LOG_ERR("Unable to add block2 option: %d", r);
      return r;
    }
    LOG_INF("Sending message (%d bytes)", request.offset);
    r = send(sock, request.data, request.offset, 0);
    if (r < 0)
    {
      LOG_ERR("Error sending request: %d", r);
      return r;
    }
    // Wait for response
    wait_for_data();
    int rcvd = recv(sock, coap_data_buffer, MAX_COAP_MSG_LEN, MSG_DONTWAIT);
    if (rcvd == 0)
    {
      // End of file
      LOG_ERR("No data received from server: %d", rcvd);
      return -EIO;
    }
    if (rcvd < 0)
    {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
      {
        return 0;
      }
      return -errno;
    }

    r = coap_packet_parse(&reply, coap_data_buffer, rcvd, NULL, 0);
    if (r < 0)
    {
      LOG_ERR("Invalid CoAP packet received: %d", r);
      return r;
    }
    r = coap_update_from_block(&reply, &blk_ctx);

    size_t len = rcvd - reply.hdr_len - reply.opt_len;
    total_size += len;
    last_block = (coap_next_block(&reply, &blk_ctx) == 0);
    r = callback(last_block, total_size - len,
                 coap_data_buffer + reply.offset,
                 len);
    if (r != 0)
    {
      LOG_INF("Aborting blockwise transfer. Return value = %d", r);
      return r;
    }
  }
  return 0;
}