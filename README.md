# MicroPythonOS for Waveshare ESP32-S3-RLCD-4.2

A port of [MicroPythonOS](https://github.com/MicroPythonOS/MicroPythonOS) to the Waveshare ESP32-S3-RLCD-4.2 development board, featuring a 4.2" Reflective LCD display.

## Hardware

- **Board**: Waveshare ESP32-S3-RLCD-4.2
- **MCU**: ESP32-S3 with 8MB PSRAM (Octal)
- **Display**: 4.2" ST7305 Reflective LCD, 300×400 pixels, I1 (1-bit) color format
- **Buttons**: BOOT (GPIO0), KEY (GPIO18)
- **SD Card**: SPI bus shared with display (CS=GPIO10)

## Features

- LVGL-based UI with I1 color format support
- ST7305 display driver with hardware acceleration
- Button input with multi-click detection
- Battery voltage monitoring via ADC
- OTA update support

## Building

### Prerequisites

- Python 3.x
- ESP-IDF v5.1.2
- PlatformIO (optional)

### Build Command

```bash
./scripts/build_mpos.sh rlcd_42
```

This builds MicroPythonOS for the ESP32-S3 with SPIRAM_OCT variant, 16MB flash, and OTA support.

### Other Targets

```bash
./scripts/build_mpos.sh unix        # Desktop build for testing
./scripts/build_mpos.sh macOS       # macOS native build
./scripts/build_mpos.sh esp32s3     # Generic ESP32-S3
```

## Flashing

After building, flash the firmware to the board:

```bash
python3 lvgl_micropython/lib/micropython/ports/esp32/build-ESP32_GENERIC_S3-SPIRAM_OCT/esptool.py --port /dev/ttyACM0 write_flash 0x0 lvgl_micropython/lib/micropython/ports/esp32/build-ESP32_GENERIC_S3-SPIRAM_OCT/firmware.bin
```

## Project Structure

```
├── c_mpos/                    # C modules with MicroPython bindings
│   └── src/
│       ├── st7305.c          # ST7305 display driver
│       └── st7305.h          # Driver header
├── internal_filesystem/       # MicroPython filesystem layout
│   └── lib/mpos/board/
│       └── waveshare_rlcd_42.py  # Board initialization
├── lvgl_micropython/          # LVGL + MicroPython submodule
├── scripts/
│   └── build_mpos.sh         # Build script
└── manifests/                 # Frozen module manifests
```

## Board Pinout

| Function | GPIO |
|----------|------|
| SPI MOSI | 11 |
| SPI SCK | 12 |
| SPI MISO | 13 |
| Display CS | 40 |
| Display DC | 5 |
| Display RST | 41 |
| SD Card CS | 10 |
| Button KEY | 18 |
| Button BOOT | 0 |

## License

See [LICENSE](LICENSE) for details.
