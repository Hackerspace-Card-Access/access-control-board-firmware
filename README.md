# access-control-board-firmware

Firmware for embedded access control boards that handle RFID/keycard input, door strike actuation, and MQTT-based communication.

## 🔧 Features

- Reads card input via Wiegand or RS485
- Publishes card reads to an MQTT topic (`reader/announce`)
- Listens for door actuation commands via MQTT (`door/actuate`)
- Fail-open fallback mode with site-code validation if MQTT is offline
