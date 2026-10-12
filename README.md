# 270° Tracking Camera

A motion-triggered surveillance camera that pans across a 270° field of view, photographs whatever moved, archives every frame on its own SD card, and alerts you twice over — once by SMS and once by Telegram.

The firmware runs on **two microcontrollers** that share one repository and one code base. A compile-time flag selects which board the current upload targets, so the same project flashes both boards by swapping the PlatformIO environment.

| Node | Board | Environment | Compile flag |
| --- | --- | --- | --- |
| Servo / capture node | ESP32 DevKit (ESP32-WROOM-32) | `servo_node` | `NODE_FLAG_SERVO` |
| Camera node | AI-Thinker ESP32-CAM (OV2640) | `camera_node` | `NODE_FLAG_CAMERA` |

---

## Table of Contents

- [Why Two Microcontrollers](#why-two-microcontrollers)
- [Features](#features)
- [How It Works](#how-it-works)
- [Architecture](#architecture)
- [Project Structure](#project-structure)
- [Hardware Requirements](#hardware-requirements)
- [Wiring and Pin Map](#wiring-and-pin-map)
- [Getting Started](#getting-started)
- [Credentials and Secrets](#credentials-and-secrets)
- [Configuration Reference](#configuration-reference)
- [Troubleshooting](#troubleshooting)
- [Design Notes and Limitations](#design-notes-and-limitations)
- [Contributors](#contributors)
- [License](#license)

---

## Why Two Microcontrollers

The ESP32-CAM is the only cheap module that gives you a camera, a micro-SD slot and Wi-Fi in a single board — but it pays for that with almost every pin it has.

**1. The camera bus is parallel, and it is huge.** The OV2640 is not a serial peripheral: it needs 8 data lines (`D0`–`D7`) plus `PCLK`, `VSYNC`, `HREF`, `XCLK`, the `SIOD`/`SIOC` control pair and `PWDN`. On the AI-Thinker board those land on GPIO 0, 5, 18, 19, 21, 22, 23, 25, 26, 27, 32, 34, 35, 36 and 39 — fifteen pins gone before anything else is connected.

**2. The SD card takes the rest.** The socket is wired to the SDMMC bus, which the AI-Thinker uses in 4-bit mode: GPIO 2, 4, 12, 13, 14 and 15. Saving images to the card — which is exactly why this project archives to SD rather than relying on the network — claims all six.

| Function | GPIO |
| --- | --- |
| Camera (OV2640) | 0, 5, 18, 19, 21, 22, 23, 25, 26, 27, 32, 34, 35, 36, 39 |
| micro-SD (4-bit) | 2, 4, 12, 13, 14, 15 |
| Status / illumination LED | 4 |
| UART0 console (debug) | 1, 3 |
| PSRAM chip select (PSRAM modules) | 16 |

**3. That leaves nothing for the robot.** This project needs, beyond the camera and the card:

- 1 PWM pin for the pan servo
- 2 digital inputs for the PIR sensors
- 2 pins for a `UART` to talk to the SIM800L
- 1 output for the capture request

That is six extra pins, and the ESP32-CAM has none to give. Running the card in **1-bit mode** (GPIO 14 `CLK`, 15 `CMD`, 2 `D0`) frees GPIO 4, 12 and 13, and this firmware spends GPIO 4 on the flash LED straight away. The capture-trigger input sits on GPIO 3 — the pin the camera sketch already watched — which is also `UART0 RX`, so the serial console becomes output-only while the wire is attached. GPIO 12 is a strapping pin best left alone, GPIO 16 belongs to PSRAM, and GPIO 13 is the only pin left over. There is no arrangement of those pins that also fits a servo, two PIRs and a GSM modem — and the SIM800L needs a *second* UART, which the ESP32-CAM simply does not have (its only UART is the console).

**So the work is split along the line of what each board is uniquely good at:**

- **ESP32-CAM** — camera, micro-SD archive, Wi-Fi/Telegram upload. The three things no other cheap board does.
- **ESP32 DevKit** — pan servo, PIR sensing, capture request, SIM800L SMS. It has plenty of GPIO, two spare hardware UARTs, and does not care that it has no camera.

The interface between them is a **single GPIO wire**: the DevKit pulls it low to ask for a photo, the camera node answers by capturing one. Everything else is independent — each board runs its own firmware, and either one can be re-flashed or debugged on its own.

A useful side effect is redundancy in alerting: the camera node reports over Wi-Fi/Telegram, while the servo node reports over GSM/SMS. If there is no Wi-Fi, the photos still land on the SD card and the SMS still goes out; if there is no SIM, Telegram still gets the frame.

---

## Features

- **270° pan coverage** — the servo sweeps three fixed stations (45°, 90°, 135°), so motion on either side is centred before the shot is taken.
- **Dual-PIR direction sensing** — the node knows which side moved, so a one-sided event costs one photo instead of three.
- **Single-wire inter-MCU trigger** — interrupt-driven on the camera side, so a request is never lost while a previous frame is being written or uploaded.
- **Local SD archive first** — every frame is written to the card before upload, so evidence survives a network outage, a dead hotspot or a Telegram hiccup.
- **Telegram push notification** — the same JPEG is posted to a chat for an instant, phone-side alert with the image attached.
- **GSM SMS alert** — the servo node reports the event over the mobile network, independent of Wi-Fi entirely.
- **Non-fatal networking** — the camera node captures and archives with no Wi-Fi at all, and reconnects on an interval instead of blocking forever.
- **Alert cooldown** — one motion event produces one SMS, not a burst.
- **Fail-fast configuration** — the build stops with a clear `#error` if the secrets file is missing or no node flag was chosen.
- **Secrets kept out of Git** — credentials live in `include/secrets.h`, which is git-ignored, with a committed `include/secrets.example.h` template.

---

## How It Works

### Servo node (ESP32 DevKit)

`servoNodeUpdate()` runs continuously:

1. Read both PIR sensors.
2. **Neither triggered** → centre the servo at 90° and wait for motion.
3. **One side triggered** → aim at that side (left PIR → 45°, right PIR → 135°), then request a photo.
4. **Both triggered** → sweep 45° → capture, 90° → capture, 135° → capture.
5. Send one SMS reporting the event.
6. Ignore further motion for `ALERT_COOLDOWN_MS` (10 s) so a single event cannot spam the phone.

A "capture" is a button press as far as the camera node is concerned: the servo node has already aimed the camera and given the servo time to travel, so the timing stays on the side that owns the motion detection.

### Camera node (ESP32-CAM)

`cameraNodeUpdate()` is idle until the trigger line falls:

1. Interrupt latches the request (never lost, even mid-upload).
2. Switch on the illumination LED and let it settle.
3. Capture one JPEG from the OV2640.
4. Write it to the SD card as `IMG_0001.JPG`, `IMG_0002.JPG`, … — the next free index is discovered at boot, so numbering survives restarts without needing a clock.
5. POST the same buffer to Telegram as `multipart/form-data` and check the HTTP status line.
6. Release the frame buffer and switch the LED off.

### Trigger line timing

```
        servo node                                    camera node
  aim 45° ──┐
            │ 1 s travel
            ├─ capture line LOW ───────────────────▶ falling edge → ISR latches request
            │                                       capture → SD write → Telegram upload
            │ 2 s hold
            └─ capture line HIGH ──────────────────▶ line idle, ready for the next request
```

The line idles HIGH. Holding it LOW for 2 s gives the camera node a window it can see even if it is busy when the edge arrives, and it is long enough that electrical noise cannot fake a request.

---

## Architecture

```
     ┌──────────────────────────────┐        ┌───────────────────────────────┐
     │  SERVO NODE — ESP32 DevKit   │        │  CAMERA NODE — ESP32-CAM      │
     │  env: servo_node             │        │  env: camera_node             │
     │                              │        │                               │
     │  pir_sensor ─── PIR L / R    │        │  trigger_input ◀── GPIO3      │
     │  servo_driver ─ pan servo    │        │  camera_driver ─ OV2640 + LED │
     │  capture_trigger ─ GPIO32 ───┼───────▶│  image_storage ─ micro-SD     │
     │  sms_notifier ─ SIM800L      │  one   │  wifi_link ───── 802.11       │
     │  servo_node ─ orchestration  │  GPIO  │  telegram_client ─ TLS POST   │
     │                              │        │  camera_node ─ orchestration  │
     └──────────────┬───────────────┘        └───────────────┬───────────────┘
                    │ UART1 @9600                            │ HTTPS 443
                    ▼                                        ▼
             SIM800L ── GSM ──▶ SMS alert            api.telegram.org ──▶ photo
```

Both firmwares come out of the same `src/` tree and the same `include/config.h`; the environment in [platformio.ini](./platformio.ini) decides which one is compiled.

---

## Project Structure

```
270-degree-tracking-camera/
├── platformio.ini          # The two environments + their node flags
├── include/
│   ├── config.h            # Node selection, pins, timings (single source of truth)
│   ├── secrets.example.h   # Credentials template  (committed)
│   ├── secrets.h           # Real credentials       (git-ignored — never commit)
│   ├── servo_node.h        # Servo node entry points
│   ├── pir_sensor.h        # Dual PIR sampling
│   ├── servo_driver.h      # Pan servo
│   ├── capture_trigger.h   # Capture request line (output side)
│   ├── sms_notifier.h      # SIM800L AT-command driver
│   ├── camera_node.h       # Camera node entry points
│   ├── trigger_input.h     # Capture request line (input side)
│   ├── camera_driver.h     # OV2640 init/capture + LED
│   ├── image_storage.h     # micro-SD archive
│   ├── wifi_link.h         # Optional Wi-Fi with retry
│   └── telegram_client.h   # Multipart photo upload
├── src/
│   ├── main.cpp            # Entry point; dispatches on the active node
│   ├── servo_node.cpp      # Motion → aim → capture → alert
│   ├── pir_sensor.cpp
│   ├── servo_driver.cpp
│   ├── capture_trigger.cpp
│   ├── sms_notifier.cpp
│   ├── camera_node.cpp     # Request → capture → archive → upload
│   ├── trigger_input.cpp
│   ├── camera_driver.cpp
│   ├── image_storage.cpp
│   ├── wifi_link.cpp
│   └── telegram_client.cpp
├── .gitignore              # Ignores .pio/ and include/secrets.h
├── LICENSE
└── README.md
```

`src/main.cpp` stays deliberately thin: it configures the selected node, then calls that node's update function from `loop()`.

---

## Hardware Requirements

| Component | Notes |
| --- | --- |
| AI-Thinker ESP32-CAM | OV2640 camera module, 4 MB flash, PSRAM version strongly recommended (VGA capture needs it) |
| ESP32 DevKit (ESP32-WROOM-32) | Any 38-pin ESP32 dev board with USB |
| micro-SD card | FAT32, 32 GB or smaller. Almost any class works; Class 10 writes VGA frames comfortably |
| Pan servo | SG90/MG90S class. **Power it from its own 5 V rail**, not from the ESP32 regulator |
| 2 × PIR sensor | HC-SR501 or similar, 3.3 V-compatible output |
| SIM800L | UART GSM modem for the SMS alert |
| USB-to-serial adapter | FTDI/CP2102 — the ESP32-CAM has no USB socket |
| 5 V supply, ≥ 2 A | Camera inrush current will brown out a weak supply |
| 4 V supply, ≥ 2 A | SIM800L wants ~4 V and peaks around 2 A while transmitting. Its own rail, common ground |

> **Common ground is mandatory.** The capture-trigger wire is referenced to ground, so the two boards must share it, along with the servo, SIM800L and LED supplies.

---

## Wiring and Pin Map

### Servo node — ESP32 DevKit (`platformio.ini → servo_node`)

| Signal | GPIO | Direction | Notes |
| --- | --- | --- | --- |
| PIR left (drives 45°) | 35 | in | Input-only pin. Was `pir2` in the original sketch |
| PIR right (drives 135°) | 34 | in | Input-only pin. Was `pir1` in the original sketch |
| Pan servo signal | 23 | out | 50 Hz PWM, 500–2400 µs |
| Capture trigger | 32 | out | Idle HIGH, LOW = "take a photo" |
| SIM800L RX (ESP32 TX) | 17 | out | UART1, 9600 baud |
| SIM800L TX (ESP32 RX) | 16 | in | UART1, 9600 baud |

If your PIRs are physically swapped, exchange `PIN_PIR_LEFT` and `PIN_PIR_RIGHT` in [config.h](./include/config.h).

### Camera node — AI-Thinker ESP32-CAM (`platformio.ini → camera_node`)

| Signal | GPIO | Notes |
| --- | --- | --- |
| Capture trigger input | 3 | Wired to the servo node's GPIO32. Shared with `UART0 RX` |
| Illumination LED | 4 | Onboard flash LED |
| SD card (SDMMC 1-bit) | 14 CLK, 15 CMD, 2 D0 | 1-bit mode keeps GPIO 4 free for the LED |
| Camera XCLK / SIOD / SIOC | 0 / 26 / 27 | Fixed on this board |
| Camera D0–D7 | 5, 18, 19, 21, 36, 39, 34, 35 | Fixed on this board |
| Camera VSYNC / HREF / PCLK / PWDN | 25 / 23 / 22 / 32 | Fixed on this board |
| Unused / available | 1, 12, 13 | GPIO 1 is the console TX; GPIO 13 is the obvious alternative trigger pin |

### Inter-board link

| From | To | Signal |
| --- | --- | --- |
| Servo node GPIO32 (output) | Camera node GPIO3 (input) | Capture request: idle HIGH, LOW = capture |
| Servo node GND | Camera node GND | Common reference — required |

Both boards run at 3.3 V logic, so no level shifting is needed.

---

## Getting Started

### 1. Prerequisites

- [VS Code](https://code.visualstudio.com/) with the [PlatformIO IDE extension](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide), or PlatformIO Core 6.x on the CLI
- `platform = espressif32`, `framework = arduino`
- Dependencies are declared per environment and installed automatically on the first build:
  - Servo node: `madhephaestus/ESP32Servo`
  - Camera node: `espressif/esp32-camera`

### 2. Create your credentials file

Nothing compiles until this exists — the build deliberately stops with:

```
#error "Missing include/secrets.h - copy include/secrets.example.h to include/secrets.h ..."
```

```bash
copy include\secrets.example.h include\secrets.h     # Windows
cp include/secrets.example.h include/secrets.h       # macOS / Linux
```

Then edit `include/secrets.h` — see [Credentials and Secrets](#credentials-and-secrets).

### 3. Build and upload

Pick the environment in the PlatformIO toolbar and press **Upload**, or use the CLI:

```bash
pio run -e servo_node  -t upload
pio run -e camera_node -t upload
```

Or build both without uploading:

```bash
pio run -e servo_node -e camera_node
```

Each environment passes its flag (`-D NODE_FLAG_SERVO` / `-D NODE_FLAG_CAMERA`), which [config.h](./include/config.h) turns into `NODE_IS_SERVO` / `NODE_IS_CAMERA`. `build_src_filter` also keeps each environment from compiling the other node's modules, so the servo build never pulls in the camera stack.

**Uploading to the ESP32-CAM:** it has no USB socket. Connect an FTDI/CP2102 adapter (adapter TX → board U0R, adapter RX → board U0T, 5 V and GND), tie **GPIO0 to GND**, press the board's reset button, start the upload, then release GPIO0 and reset again to run the firmware. **Disconnect the capture-trigger wire first** — the servo node actively drives GPIO 3, which is the ESP32-CAM's `UART0 RX` and is used by the bootloader, so a connected trigger line will block the upload. If the upload is flaky, lower `upload_speed` in [platformio.ini](./platformio.ini) from `460800` to `115200`.

### 4. Run

Open both serial monitors at 115200 baud if you want the full picture:

```
[ServoCaptureNode] starting up
[pir] left=0 right=1                 ← right-hand sensor sees motion
[sms] alert sent
[CameraNode] capture requested by the servo node
[sd] stored IMG_0007.JPG (24188 bytes)
[telegram] HTTP/1.1 200 OK
[camera] 24188 byte frame handled in 3120 ms
```

---

## Credentials and Secrets

Real credentials live in `include/secrets.h`, which **is listed in [.gitignore](./.gitignore) and must never be committed**. It was seeded with the values that used to be hard-coded in the original sketches, so the firmware keeps working exactly as before — but those values now stay on your machine.

| Key | Used by | Where to get it |
| --- | --- | --- |
| `WIFI_SSID`, `WIFI_PASSWORD` | camera node | Your 2.4 GHz access point. The ESP32 is 2.4 GHz only |
| `TELEGRAM_BOT_TOKEN` | camera node | Create a bot with [@BotFather](https://t.me/BotFather) |
| `TELEGRAM_CHAT_ID` | camera node | Send `/start` to your bot, then read `https://api.telegram.org/bot<token>/getUpdates` |
| `ALERT_PHONE_NUMBER` | servo node | Destination for the SMS alert, E.164 format (`+2348012345678`) |

Before pushing the project to GitHub:

```bash
git check-ignore -v include/secrets.h    # must print the .gitignore match
git status --porcelain                   # secrets.h must not appear
```

> **Rotate anything that has already been exposed.** The original sketches that seeded this project carried a live Telegram bot token in plain source, so that token should be treated as compromised: talk to @BotFather, have it revoked, and put only the replacement in `secrets.h`.

---

## Configuration Reference

Everything tunable lives in [include/config.h](./include/config.h), split into one block per node.

### Servo node

| Constant | Default | Meaning |
| --- | --- | --- |
| `PIN_PIR_LEFT` / `PIN_PIR_RIGHT` | 35 / 34 | PIR inputs |
| `PIN_SERVO` | 23 | Servo signal |
| `SERVO_ANGLE_LEFT` / `_CENTER` / `_RIGHT` | 45 / 90 / 135 | The three stations |
| `SERVO_MIN_PULSE_US` / `_MAX_PULSE_US` | 500 / 2400 | Servo pulse width range |
| `SERVO_SETTLE_MS` | 1000 | Travel time allowed before a capture |
| `PIN_CAPTURE_TRIGGER` | 32 | Trigger line to the camera node |
| `CAPTURE_TRIGGER_HOLD_MS` | 2000 | How long the request is held low |
| `ALERT_SMS_MESSAGE` | `"Motion detected!"` | SMS body |
| `ALERT_COOLDOWN_MS` | 10000 | Motion is ignored for this long after an alert |
| `SIM800_*` | see file | UART pins, baud rate and response timeouts |

### Camera node

| Constant | Default | Meaning |
| --- | --- | --- |
| `PIN_CAPTURE_TRIGGER` | 3 | Trigger input from the servo node (shared with `UART0 RX`) |
| `PIN_FLASH_LED` | 4 | Illumination LED |
| `CAM_FRAMESIZE_PSRAM` | `FRAMESIZE_VGA` | Frame size when PSRAM is present |
| `CAM_JPEG_QUALITY_PSRAM` | 10 | JPEG quality (0–63, lower is better) |
| `CAM_FRAMESIZE_NO_PSRAM` | `FRAMESIZE_QVGA` | Fallback frame size without PSRAM |
| `SD_MOUNT_POINT` / `SD_1BIT_MODE` | `/sdcard` / 1 | Card mount and bus width |
| `IMAGE_NAME_PREFIX` / `_DIGITS` | `IMG_` / 4 | Produces `IMG_0007.JPG` |
| `WIFI_CONNECT_TIMEOUT_MS` / `_RETRY_INTERVAL_MS` | 15000 / 30000 | Link attempt budget and retry gap |
| `TELEGRAM_RESPONSE_TIMEOUT_MS` | 10000 | How long to wait for the API's status line |

---

## Troubleshooting

| Symptom | Likely cause / fix |
| --- | --- |
| `Missing include/secrets.h` during build | Copy `include/secrets.example.h` to `include/secrets.h`. This is intentional |
| `No node selected` during build | Build through a PlatformIO environment (`servo_node` / `camera_node`) rather than a bare `pio run` on a source file |
| ESP32-CAM will not accept an upload | It has no USB: use an FTDI adapter, tie GPIO0 to GND, reset, upload, then release GPIO0 and reset. **Disconnect the trigger wire** — the servo node actively drives GPIO 3, which the bootloader needs. Try `upload_speed = 115200` if it is still flaky |
| ESP32-CAM reboots when it captures | Weak 5 V supply — the LED plus camera inrush draw a lot of current at once. Give it ≥ 2 A of headroom. If it still resets, disable the brown-out detector by adding `WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);` at the top of `cameraNodeInit()` |
| `[camera] init failed (0x...)` | Ribbon cable not seated, or too little current. The node restarts itself to retry |
| `[sd] no card mounted` | Card must be FAT32 and ≤ 32 GB. Try re-seating it, or slow the bus: add `-DBOARD_MAX_SDMMC_FREQ=SDMMC_FREQ_DEFAULT` to the camera environment's `build_flags` |
| Images are saved but never reach Telegram | Wi-Fi is down (check the `[wifi]` lines), or the token/chat id is wrong. A bot will not deliver to a chat until you have sent it `/start` |
| PIR triggers but no photo | Check the single trigger wire (servo GPIO32 → camera GPIO3) and the common ground. Both serial monitors will tell you which side stopped |
| Servo jitters or the DevKit resets | The servo is drawing from the board's regulator. Give it its own 5 V supply |
| SMS never arrives | SIM800L needs ~4 V at up to 2 A, its own rail, a registered SIM, and crossed RX/TX against the ESP32. The serial log now reports exactly which AT step failed |
| Photos are dark | The onboard LED is a small status flash, not a real illuminator. Light the scene externally, or remove the LED calls from `handleCaptureRequest()` if it is not helping |

---

## Design Notes and Limitations

- **SD runs in 1-bit mode on purpose.** Full 4-bit mode is faster, but it also claims GPIO 4, which the flash LED needs. VGA JPEG writes are comfortably fast enough on 1-bit. If you switch `SD_1BIT_MODE` to 0, the build emits a warning: move `PIN_FLASH_LED` yourself.
- **The trigger shares `UART0 RX`.** GPIO 3 is the pin the camera sketch already watched, and it is the only header pin that was genuinely spare. The cost is the serial console's input — the node still logs at 115200, but it cannot be driven over serial, and the wire has to come off to flash the board. `PIN_CAPTURE_TRIGGER` can be moved to the free GPIO 13 in one line if you would rather keep the console.
- **The trigger is interrupt-driven, not polled.** A capture takes seconds (SD write + TLS upload). Polling would miss any request that arrived in that window; an ISR latches them instead.
- **Requests are coalesced.** The latch is a single flag, so several pulses arriving during one long upload produce one follow-up capture, not a backlog of shots.
- **Archiving happens before uploading.** The SD write is attempted first and its result decides the Telegram filename, so a network failure never costs you the image.
- **Filenames are not timestamps.** The ESP32-CAM has no clock and the firmware deliberately avoids NTP, so images are numbered sequentially and the next free index is recovered from the card at boot.
- **The servo node's capture sequence is blocking by design.** It owns the timing of the sweep, and nothing else shares that board. The alert cooldown, however, is non-blocking so the PIR state keeps printing while alerts are suppressed.
- **One-way trigger.** The camera node does not report success back to the servo node; the request line is a request, not a handshake. The result is visible in the camera node's serial log and in whatever reached Telegram/SD.
- **`setInsecure()` is used for TLS.** Certificate pinning needs a valid clock, which this board does not have. The traffic is still encrypted; the certificate simply is not validated.
- **The ESP32 is 2.4 GHz only.** SSIDs on the 5 GHz band will never connect.

---

## Contributors

- **Abdullateef** ([`aBDULLATEEF1247`](https://github.com/aBDULLATEEF1247)) — ESP32-CAM firmware, micro-SD storage handler, and Telegram bot pipeline.

---

## License

Released under the MIT License — see [LICENSE](./LICENSE).
