# LilyGo T5S3 V2.4 mod 

1) Removed 74HCT4094D  
- <img src="/assets/T5-S3-mod.png" width="200" title="T5-S3-mod" alt="T5-S3-mod"/>

4) EPD connected to the following ESP32_S3 GPIO:  
`EPD_LE=(GPIO_NUM_48)`  
`EPD_STV=(GPIO_NUM_45)`  
`EPD_OE=(GPIO_NUM_12)`  
`EPD_MODE=(GPIO_NUM_13)`  
`EPD_PWR=(GPIO_NUM_39) ; //CS`  

# LilyGo-EPD-4-7-OWM-Weather-Display

Open Weather Map weather station using [LilyGo EPD 4.7" display](https://bit.ly/3exI3Hb)

[![Presetation video](assets/001.png)](https://www.youtube.com/watch?v=TQaVQcld1Pk)

# Home Assistant MQTT through Cloudflare

After each successful weather read the station can publish current conditions, air quality, and the forecast to a Home Assistant MQTT broker. The broker stays behind Cloudflare. The ESP32 opens a normal HTTPS WebSocket (`wss`, port 443) so Cloudflare can route the connection, read the Access headers, and apply a Zero Trust policy. MQTT itself starts only after that handshake.

Leave `home_assistant_server_host` empty to keep MQTT off. The display and deep sleep still run.

Enter these on the Wi-Fi setup page (`LilyGo-T5S3-weather-wifi`). They are stored with the other station settings.

| Field | What to enter |
| --- | --- |
| `wifi_name` | Wi-Fi network the station joins after setup |
| `wifi_password` | Password for that network |
| `home_assistant_server_host` | Public Cloudflare hostname. Add a path when the broker is not at `/mqtt` (for example `ha.example.com/mqtt`). `wss://` is optional. Port is 443. |
| `CF-Access-Client-Id` | Cloudflare Access service-token client id |
| `CF-Access-Client-Secret` | Cloudflare Access service-token secret |
| `mqtt-user` | MQTT username on the Home Assistant broker |
| `mqtt-password` | MQTT password on the Home Assistant broker |

`CF-Access-Client-Id` and `CF-Access-Client-Secret` are sent together as HTTP headers on the WebSocket upgrade, along with `Sec-WebSocket-Protocol: mqtt`. Cloudflare checks those headers before it proxies the socket. The station then sends a normal MQTT login with `mqtt-user` and `mqtt-password`. Set both Access fields, or neither. One without the other is skipped.

The retained document is published to `lilygo/epd47/<id>/weather`. It includes current conditions, air quality, the forecast, battery percentage, battery voltage, and the wake count. Home Assistant discovery sensors are published for the same readings. The Access secret and the MQTT password are not printed on the serial log.

# License

This code created by https://github.com/G6EJD/ is using the GPLv3 https://github.com/Xinyuan-LilyGO/LilyGo-EPD47 library to handle the display and as such falls into the GPLv3 license itself. This situation is described in the https://www.gnu.org/licenses/gpl-faq.html#IfLibraryIsGPL

> If a library is released under the GPL (not the LGPL), does that mean that any software which uses it has to be under the GPL or a GPL-compatible license?

> Yes, because the program actually links to the library. As such, the terms of the GPL apply to the entire combination. The software modules that link with the library may be under various GPL compatible licenses, but the work as a whole must be licensed under the GPL.

This means that the original proprietary license that G6EJD tried to enforce is unlawful as it is not compatible with the GPLv3.

This fork fixes the problem by setting the correct license for the code while keeping the attribution and all the copyright of the original creator.

# Compiling and flashing

Edit `owm_credentials.h` and enter OWM API key as well as the location for which you want to display the weather data

To compile you will need following libraries

* https://github.com/Xinyuan-LilyGO/LilyGo-EPD47
* https://github.com/bblanchon/ArduinoJson

In board manager choose ESP32 Dev Module with PSRAM Enabled
