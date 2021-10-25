# Span Zephyr/PlatformIO sample

This is a sample project that works both on local etherned (with the external
CoAP interface for devices) or via a cellular IoT modem (with the CIoT CoAP
interface). The project uses PlatformIO but should build equally well for any
supported device in Zephyr. The file `zephyr/prj.conf` contains the required
configure flags for the build.

Run and deploy to the device with `pio run --target=upload`. Use
`pio device monitor --raw` to view the log.

The ethernet-connected devices uses DTLS with client certificates to
authenticate and verify the client connection.

The CIoT devices may elect to use unencrypted UDP for the CoAP service. This
makes deployments a bit easier and less resource hungry since they won't need
a client certificate, DNS or DHCP support and everything can be configured via
Span.

The project is developed on a STM32 F429zi board but it should be relatively
easy to modify it to run on any board with ethernet/wifi connectivity or a
cellular IoT modem (like the nRF91 from Nordic Semiconductor) as long as it
has a rudimentary socket support.
