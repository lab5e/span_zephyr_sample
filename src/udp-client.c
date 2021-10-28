#include <stdio.h>
#include <errno.h>

#include <logging/log.h>
#include <zephyr.h>

#include <net/coap.h>
#include <net/net_ip.h>
#include <net/socket.h>
#include <net/udp.h>

LOG_MODULE_REGISTER(udp_client, LOG_LEVEL_DBG);

#include "udp-client.h"

#include "clientcert.h"

#ifdef CLIENT_CERT
#include <net/tls_credentials.h>
#endif

int send_udp(const char *host, const int port)
{
    int ret = 0;
    int sock = 0;

    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    inet_pton(AF_INET, host, &addr.sin_addr);

#ifdef CLIENT_CERT
    /*
    ret = tls_credential_add(ROOT_CERT_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
                             root_certificate, sizeof(root_certificate));
    if (ret != 0)
    {
        LOG_ERR("Unable to add root certificate to TLS credentials: %d", ret);
    }

    // There's no constant for a client certificate but the rest of the code
    // refers to the server certificate as "own" so this looks a bit weird.
    ret = tls_credential_add(CLIENT_CERT_TAG, TLS_CREDENTIAL_SERVER_CERTIFICATE,
                             client_certificate, sizeof(client_certificate));
    if (ret != 0)
    {
        LOG_ERR("Unable to add client certificate to TLS credentials: %d", ret);
    }
    ret = tls_credential_add(CLIENT_CERT_TAG, TLS_CREDENTIAL_PRIVATE_KEY,
                             client_key, sizeof(client_key));
    if (ret != 0)
    {
        LOG_ERR("Unable to add client key to TLS credentials: %d", ret);
    }
    LOG_DBG("TLS credentials added for socket");
*/
    sock = socket(addr.sin_family, SOCK_DGRAM, IPPROTO_DTLS_1_2);
#else
    sock = socket(addr.sin_family, SOCK_DGRAM, IPPROTO_UDP);
#endif
    if (sock < 0)
    {
        LOG_ERR("Failed to create UDP socket %d", errno);
        return -errno;
    }
#ifdef CLIENT_CERT
    sec_tag_t sec_tag_list[] = {
        ROOT_CERT_TAG,
        CLIENT_CERT_TAG,
    };

    ret = setsockopt(sock, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_list,
                     sizeof(sec_tag_list));
    if (ret < 0)
    {
        LOG_ERR("Error setting TLS tag socket option: %d", errno);
        return -errno;
    }

    // Certificate verification doesn't work - it might be a memory issue or it
    // might be the missing intermediate(s). The verification works for the
    // *server* so the client is behaving as expected and it works for mbedtls on
    // ESP-IDF so I'm inclined to point a finger on impedance mismatch somewhere
    // in Zephyr
    int verify = TLS_PEER_VERIFY_OPTIONAL;
    ret = setsockopt(sock, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify));
    if (ret < 0)
    {
        LOG_ERR("Failed to set TLS_PEER_VERIFY option: %d", errno);
        ret = -errno;
    }
#endif

    LOG_INF("Connecting to UDP service on port %d...", port);
    ret = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0)
    {
        LOG_ERR("Cannot connect UDP socket: %d", ret);
        return -errno;
    }
    LOG_INF("Connected to service on port %d", port);
    k_sleep(K_MSEC(2500));
    char buffer[32];

    for (int i = 0; i < 100; i++)
    {
        sprintf(buffer, "Packet %d", i);
        ret = send(sock, buffer, strlen(buffer), 0);
        if (ret < 0)
        {
            LOG_ERR("Error sending data on socket: %d", ret);
            return ret;
        }
        LOG_INF("Sent UDP packet %d...", i);
        k_sleep(K_MSEC(250));
    }

    close(sock);
    return 0;
}