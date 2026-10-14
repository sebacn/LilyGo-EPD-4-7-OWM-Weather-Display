# Agent context

OpenWeatherMap weather station for a modified LilyGo T5 4.7" ESP32-S3 e-paper board. Read this before changing firmware.

The sketch is `OWM_EPD47_epaper_v2.72.ino`. `platformio.ini` sets `src_dir = .`, so that file is the program. Shared settings, the Wi-Fi portal, and validation live in `config.h`. Weather structs are in `forecast_record.h`. Pins are in `utilities.h` (the sketch includes this copy, not the library copy).

Do not commit unless the user asks. Do not put API keys, Wi-Fi passwords, or tokens into this file, logs meant for the user, or new sample output.

## Hardware

- Board: LILYGO T5 e-Paper 4.7 Inch ESP32-S3, touch version. Panel is ED047TC1, 960×540, 16 grayscale.
- This unit is a hardware mod: the 74HCT4094 is removed. E-paper control pins are overridden in `platformio.ini` and must stay as they are: LE 48, STV 45, OE 12, MODE 13, PWR/CS 39.
- Touch is GT911 on I2C. SDA GPIO 18, SCL GPIO 17, interrupt GPIO 47. Addresses 0x14 or 0x5D. Init matches LilyGo `examples/touch/touch.ino`: after deep-sleep wake wait about 1 s, pulse INT high, scan both addresses, `setSwapXY(true)`, `setMirrorXY(false, true)`, poll at least every 300 ms.
- GPIO 47 is not an RTC pin, so it cannot wake deep sleep. LilyGo’s documented fix is a wire from GPIO 47 to GPIO 10. Firmware wakes on GPIO 10 (active low) plus the buttons. Without that wire, touch works only during the awake window after a refresh.
- Do not call `touch.sleep()` before deep sleep. That holds INT low and stops the controller from generating a wake pulse. Leaving GT911 awake costs a few milliamps.
- GPIO 10 is also `GPIO_MOSI` in `utilities.h`. It is not the SD MOSI pin. Do not use it as SPI data for the panel or the card.
- Buttons: GPIO 0 (BOOT, EXT0) cycles the image slideshow (cat, landscape, minion). GPIO 21 (EXT1) steps to the next layout. Timed wake keeps the current layout.
- Battery ADC is GPIO 14.
- SD slot, ESP32-S3 pins in `utilities.h`: SCLK 11, MOSI 15, MISO 16, CS 42. Format the card FAT32. exFAT and NTFS are not mounted. These pins do not overlap the custom e-paper pins.
- USB serial is the onboard USB port, VID:PID `303A:1001`, usually COM6, 115200. `ARDUINO_USB_CDC_ON_BOOT=1` is required or `Serial` goes to the UART and the monitor stays empty. The port disappears in deep sleep and returns on wake. A USB/esptool reset is `ESP_SLEEP_WAKEUP_UNDEFINED`, and the sketch then opens the Wi-Fi setup portal even when saved settings are valid. The portal times out and returns to weather if `ConfigOk` is still true.

## Build and flash

```text
pio run
pio run -t upload --upload-port COM6
pio device monitor --port COM6 --baud 115200
```

Env is `T5-ePaper-S3`: `espressif32@6.11.0`, Arduino, board `T5-ePaper-S3`, PSRAM, loop stack 16384. Obsolete PlatformIO core warnings are harmless. Do not change the PlatformIO core.

If COM6 is missing, the board is asleep. Wait for a button or touch wake, then flash immediately. A full `pio run -t upload` can lose the port while it rebuilds. The image is already at `.pio/build/T5-ePaper-S3/` and can be written with esptool once the port is back.

Debug lines are `[D]` and `[I]`, prefixed with seconds since boot, flushed, and copied to the SD log when a card is mounted. `PROJ_DEBUG_ENABLE` forces `[D]` on. Do not print secrets back to the user; `settings.print()` does include keys on the serial log.

## Screens

Layout id is stored in NVS namespace `layout` key `id`, and in RTC so sleep does not forget it.

| Id | Name | Role |
|----|------|------|
| 0 | Full | Original crowded 960×540 layout |
| 1 | Main | City and clock header, large icon and temperature, five stat rows, one forecast strip. Today is the double outline |
| 2 | Graphs | 2×2 graphs |
| 3 | Details | Wind, astronomy, visibility, air quality, 3-hour forecast |
| 4 | Device | Station details and the touch-editable settings |

There is no permanent bottom menu. A touch opens a centered **Select screen** panel. Choosing a button closes it and draws that layout. A touch outside the panel closes it and leaves the layout unchanged. On Device, setting buttons work only while the panel is hidden. A touch wake opens the panel on the current layout; it does not select a layout from the wake coordinate. After each paint the firmware stays awake `TOUCH_SESSION_MS` (15 s) and extends that on each tap, then deep-sleeps.

Redraws are full clears. Do not add partial refresh. This panel corrupts partial clears (see the hardware-issue note in `assets/`).

Main’s forecast strip is only as many calendar days as fit in `max_readings` (24, about 3 days). Do not raise `max_readings` unless asked; graph day labels assume that span. `DaySlot` must stay in `forecast_record.h`. A struct used only in the `.ino` breaks Arduino prototype generation.

## Device settings

Touch can change units (`M` metric or `I` imperial), sleep minutes (5–60, step 5), wake hour, sleep hour, and debug-log enable. **Save** writes the existing Preferences namespace `mem` through `save_config_to_memory()`. Do not invent a second settings store. City, Wi-Fi, API keys, and Influx fields stay on **Web setup**, which starts the existing WiFiManager portal (`LilyGo-T5S3-weather-wifi`).

Changing units marks weather stale so the next weather layout refetches. Unsaved edits die at sleep.

The Device screen shows SD card size (`SD.cardSize()`), percent used, and free space (`SD.totalBytes()` minus `SD.usedBytes()`).

## SD logs

Mounted in `initSdCard()` after the panel init. Unmounted in `sdPrepareSleep()` before deep sleep.

- `/weather/YYYY-MM-DD.json` — one JSON array per calendar day. Each successful onecall + forecast + air read appends one object (current, air, forecast slots). A new day creates a new file.
- `/log/YYYY-MM-DD.log` — the same `[D]` and `[I]` lines as USB serial. Lines from before the clock is set go to `/log/pending.log` and are moved into that day’s file once time is valid.

Both are flushed before sleep. Skip writes when no card is present.

## Behavior to leave alone

- Time: `update`/`SetupTime` uses TimeZoneDB. The display clock is that local time. Do not “fix” timezone as a side task.
- Units: `M` is °C, hPa, m/s. `I` is °F, in, mph.
- Weather data is not kept across deep sleep. A layout change in the same awake session redraws from RAM. A fresh wake refetches, except the Device layout, which draws without Wi-Fi.
- `display_failed_mode_and_sleep()` and the long config-portal failure path must still end in `deep_sleep_start()` or `sdPrepareSleep()` so the card is unmounted and the wake pins stay armed.
- Fonts in use: OpenSans 8/10/12/18/24 bold. 24 is the largest included. `drawString` y is the top of the glyph box.
- Colors: White `0xFF`, LightGrey `0xBB`, Grey `0x88`, DarkGrey `0x44`, Black `0x00`. `write_string` draws black glyphs, so selected controls are double outlines, not white-on-black text.
