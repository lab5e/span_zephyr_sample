#include <errno.h>
#include <sys/types.h>

#include <logging/log.h>
#include <net/net_core.h>
#include <net/net_if.h>
#include <zephyr.h>

#include "networking.h"

LOG_MODULE_REGISTER(networking, LOG_LEVEL_DBG);

struct k_sem address_sem;

static struct net_mgmt_event_callback mgmt_cb;
static void net_event_handler(struct net_mgmt_event_callback *cb,
                              uint32_t mgmt_event, struct net_if *iface) {
  if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
    return;
  }
  LOG_DBG("Got IP address");
  k_sem_give(&address_sem);
}

void dhcp_init(void) {
  LOG_DBG("Starting DHCP");
  if (0 != k_sem_init(&address_sem, 0, 1)) {
    LOG_ERR("Could not initialize semaphore: %d", errno);
  }
  net_mgmt_init_event_callback(&mgmt_cb, net_event_handler,
                               NET_EVENT_IPV4_ADDR_ADD);
  net_mgmt_add_event_callback(&mgmt_cb);

  struct net_if *iface = net_if_get_default();
  net_dhcpv4_start(iface);

  LOG_DBG("Waiting for IP address...");
  k_sem_take(&address_sem, K_FOREVER);
}
