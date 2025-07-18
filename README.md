| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- | -------- | -------- |

# Access Control Node
(See the README.md file in the upper level 'examples' directory for more information about examples.)

## Overview

This access control node demonstrates basic usage of `Ethernet driver` together with `esp_netif` for access control applications. Initialization of the `Ethernet driver` is wrapped in separate [sub-component](./components/ethernet_init/ethernet_init.c) of this project to clearly distinguish between the driver's and `esp_netif` initializations. The work flow of the access control node could be as follows:

1. Install Ethernet driver
2. Attach the driver to `esp_netif`
3. Send DHCP requests and wait for a DHCP lease
4. If get IP address successfully, then you will be able to ping the device

This access control node provides a foundation for building secure access control systems with Ethernet connectivity and OTA update capabilities.

## How to use Access Control Node

### Hardware Required

To run this access control node, it's recommended that you have an official ESP32 Ethernet development board - [ESP32-Ethernet-Kit](https://docs.espressif.com/projects/esp-idf/en/latest/hw-reference/get-started-ethernet-kit.html). This access control node should also work for 3rd party ESP32 board as long as it's integrated with a supported Ethernet PHY chip. Up until now, ESP-IDF supports up to four Ethernet PHY: `LAN8720`, `IP101`, `DP83848` and `RTL8201`, additional PHY drivers should be implemented by users themselves.

Besides that, `esp_eth` component can drive third-party Ethernet module which integrates MAC and PHY and provides common communication interface (e.g. SPI, USB, etc). This access control node will take the `DM9051`, `W5500` or `KSZ8851SNL` SPI modules as an example, illustrating how to install the Ethernet driver in the same manner.

The ESP-IDF supports the usage of multiple Ethernet interfaces at a time when external modules are utilized which is also demonstrated by this access control node. There are several options you can combine:
   * Internal EMAC and one SPI Ethernet module.
   * Two SPI Ethernet modules of the same type connected to single SPI interface and accessed by switching appropriate CS.
   * Internal EMAC and two SPI Ethernet modules of the same type.

#### Pin Assignment

See common pin assignments for Ethernet examples from [upper level](../README.md#common-pin-assignments).

When using two Ethernet SPI modules at a time, they are to be connected to single SPI interface. Both modules then share data (MOSI/MISO) and CLK signals. However, the CS, interrupt and reset pins need to be connected to separate GPIO for each Ethernet SPI module.

### Configure the project

```
idf.py menuconfig
```

See common configurations for Ethernet examples from [upper level](../README.md#common-configurations).

### Build, Flash, and Run

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py -p PORT build flash monitor
```

(Replace PORT with the name of the serial port to use.)

(To exit the serial monitor, type ``Ctrl-]``.)

See the [Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html) for full steps to configure and use ESP-IDF to build projects.

## Access Control Node Output

```bash
I (394) ACCESS_CTL: Ethernet Started
I (3934) ACCESS_CTL: Ethernet Link Up
I (3934) ACCESS_CTL: Ethernet HW Addr 30:ae:a4:c6:87:5b
I (5864) esp_netif_handlers: eth ip: 192.168.2.151, mask: 255.255.255.0, gw: 192.168.2.2
I (5864) ACCESS_CTL: Ethernet Got IP Address
I (5864) ACCESS_CTL: ~~~~~~~~~~~
I (5864) ACCESS_CTL: ETHIP:192.168.2.151
I (5874) ACCESS_CTL: ETHMASK:255.255.255.0
I (5874) ACCESS_CTL: ETHGW:192.168.2.2
I (5884) ACCESS_CTL: ~~~~~~~~~~~
```

Now you can ping your ESP32 in the terminal by entering `ping 192.168.2.151` (it depends on the actual IP address you get).

## Troubleshooting

See common troubleshooting for Ethernet examples from [upper level](../README.md#common-troubleshooting).

(For any technical queries, please open an [issue](https://github.com/espressif/esp-idf/issues) on GitHub. We will get back to you as soon as possible.)
