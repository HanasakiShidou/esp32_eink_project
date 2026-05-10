# ESP32-C3 MQTT E-Ink Display Project

ESP32-C3 IoT project that controls a Waveshare 2.9" E-Ink display (EPD 2in9 V2, 128x296, monochrome) via MQTT v5 remote procedure calls over WiFi.

## Build & Flash

```bash
# Build
idf.py build

# Menuconfig (change WiFi credentials, broker URL, etc.)
idf.py menuconfig
```

**Toolchain:** ESP-IDF v5.5.3, CMake + Ninja, C++17, target: `esp32c3`

## Architecture

```
MQTT Broker (WiFi)  →  mqtt_app.c  →  mqtt_handle_request()
                                          ↓
                              SakataOnEsp32Server::handle_request()
                                          ↓
                         SakataOnEsp32ServerImpl  (RPC function bodies)
                                          ↓
                              EPaper_2in9v2  (display driver)
                                          ↓
                              Esp32Impl  (SPI + GPIO via ESP-IDF)
                                          ↓
                              2.9" E-Ink panel (128x296)
```

### MQTT Topics (QoS 2 for RPC, QoS 1 for logging)

| Topic | Direction | Purpose |
|-------|-----------|---------|
| `/logger` | Device → Broker | Log messages, online/offline status |
| `/rpc_interface/request` | Broker → Device | Incoming RPC commands |
| `/rpc_interface/response` | Device → Broker | RPC replies |

### RPC Functions (defined in `main/rpc/rpc_config.yaml`)

| ID | Name | Purpose | Status |
|----|------|---------|--------|
| 0x01 | `fillDisplayMem` | Load bitmap into internal framebuffer | done |
| 0x02 | `clearDisplay` | Clear screen to black or white | done |
| 0x03 | `setDisplayMem` | Set single pixel in framebuffer | done |
| 0x04 | `refreshDisplay` | Push framebuffer to display (full/partial) | done |
| 0x05 | `directlyDisplay` | Write bitmap directly, bypassing buffer | done |
| 0x06 | `fillAndRefresh` | Fill buffer + refresh in one call | done |
| 0x07 | `setLed` | Set status LED (RGB) | **stub — not implemented** |
| 0x08 | `getStatus` | Return device status | **stub — always returns 0** |

The `rpc_config.yaml` drives `builder.py` which generates `SakataOnEsp32Server.cpp/.h`. Hand-written code goes in `SakataOnEsp32ServerImpl.cpp/.h`.

### Pin Mapping (SPI2_HOST, 1 MHz, Mode 0)

| Signal | GPIO |
|--------|------|
| MOSI (DIN) | 3 |
| CLK | 4 |
| CS | 5 |
| DC | 2 |
| RST | 1 |
| BUSY | 0 |

## Key Files

- [main/app_main.c](main/app_main.c) — Entry point: init NVS, network, MQTT, RPC server
- [main/mqtt/mqtt_app.c](main/mqtt/mqtt_app.c) — MQTT v5 client, subscription, publish, event handling
- [main/rpc/SakataOnEsp32ServerImpl.cpp](main/rpc/SakataOnEsp32ServerImpl.cpp) — Actual RPC function implementations (framebuffer management, display calls)
- [main/rpc/rpc_config.yaml](main/rpc/rpc_config.yaml) — RPC interface definition, used by `builder.py` for codegen
- [main/eink/esp32_impl.cpp](main/eink/esp32_impl.cpp) — ESP32-C3 SPI/GPIO hardware backend implementing `HardwareAPI` interface
- [epd_cpp/lib/e-Paper/2in9v2/](epd_cpp/lib/e-Paper/2in9v2/) — EPD 2in9 V2 driver (C++ port of Waveshare C driver)
- [epd_cpp/lib/Hardware/hardwareapi.hpp](epd_cpp/lib/Hardware/hardwareapi.hpp) — Abstract hardware interface (`DigitalWrite`, `SPIWriteByte`, `Delay`, etc.)
- [epd_cpp/lib/e-Paper/e-paper.hpp](epd_cpp/lib/e-Paper/e-paper.hpp) — Abstract display base class `EPaper_API`
- [main/Kconfig.projbuild](main/Kconfig.projbuild) — Adds `BROKER_URL` menuconfig item

## Project Conventions

- **Logging:** Use ESP-IDF `ESP_LOGI(TAG, ...)` / `ESP_LOGE(TAG, ...)` with module-specific tags
- **Memory:** `mqtt_log(char* msg)` takes ownership of the pointer (must be `malloc`'d, will be `free`'d internally). `mqtt_send_response(char* data, int length)` does not take ownership
- **Language:** C for MQTT module and app_main, C++ for RPC and display drivers. Header guards in C use `#pragma once`, C headers compatible with both
- **RPC codegen:** If you change `rpc_config.yaml`, re-run `main/rpc/builder.py` to regenerate server stubs
- **The `epd_cpp/` directory** is a standalone project with its own git repo (branch `dev_esp32impl`). ESP32-specific code lives in `main/eink/`, not in `epd_cpp/`
- **Partition table:** Uses `partitions_singleapp_large.csv` (single large app partition, no OTA)
- **Build config:** `CONFIG_MQTT_PROTOCOL_5=y`, minimal build with only required ESP-IDF components
