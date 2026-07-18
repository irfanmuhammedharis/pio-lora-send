# pio-lora-send

PlatformIO/Arduino port of Zephyr's `samples/drivers/lora/send` (and `receive`)
using [RadioLib](https://github.com/jgromes/RadioLib), for nRF52840 boards.

Radio parameters match the Zephyr samples (865.1 MHz, BW 125 kHz, SF10, CR 4/5,
preamble 8, private sync word, +4 dBm), so the firmware interoperates with
Zephyr boards running the same samples.

## Environments

- `xiao_tx` / `xiao_rx` (defaults) — Seeed XIAO nRF52840 (Sense) + SX1262
  (Wio-SX1262 wiring: CS=D4, DIO1=D1, RST=D2, BUSY=D3). Sender / receiver roles.
  Uses the project-local board def in `boards/seeed_xiao_nrf52840_sense.json`
  (SoftDevice S140 v7.3.0 — matches the Seeed bootloader; the required
  `nrf52840_s140_v7.ld` was added to the installed Adafruit core from Seeed's
  Adafruit_nRF52_Arduino fork, along with the XIAO variant files).
- `rak4631` — RAK WisBlock RAK4631 (needs the RAKwireless WisBlock patch in
  `~/.platformio`, already applied on this machine).
- `feather_sx1276` — Adafruit Feather nRF52840 + external SX1276/RFM95.

## Usage

```bash
pio run -e xiao_tx -t upload --upload-port /dev/ttyACM0   # flash sender
pio run -e xiao_rx -t upload --upload-port /dev/ttyACM1   # flash receiver
pio device monitor -p /dev/ttyACM1 -b 115200              # watch received packets
```

Verified end-to-end: receiver prints `Received 12 bytes: helloworld N` with
RSSI/SNR for every packet the sender emits at 1 s intervals.
