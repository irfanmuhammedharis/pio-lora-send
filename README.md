# pio-lora-send

PlatformIO/Arduino port of Zephyr's `samples/drivers/lora/send` (and `receive`)
using [RadioLib](https://github.com/jgromes/RadioLib), for nRF52840 boards.

Radio parameters match the Zephyr samples (865.1 MHz, BW 125 kHz, SF10, CR 4/5,
preamble 8, private sync word, +4 dBm), so the firmware interoperates with
Zephyr boards running the same samples.

## Environments

- `xiao_a` / `xiao_b` (defaults) — Seeed XIAO nRF52840 (Sense) + SX1262
  (Wio-SX1262 wiring: CS=D4, DIO1=D1, RST=D2, BUSY=D3, TCXO on DIO3 @1.8V).
  Serial chat over LoRa: lines typed into either board's serial monitor go
  out over the air and print on the other board with RSSI/SNR. The chat
  rides on a continuous ping-pong of `~` keepalives (node A initiates,
  node B replies) because long idle receive windows leave the SX1262 deaf
  (RX-timeout errata) — every interrupt/polling chat variant without the
  constant traffic failed.
  Uses the project-local board def in `boards/seeed_xiao_nrf52840_sense.json`
  (SoftDevice S140 v7.3.0 — matches the Seeed bootloader; the required
  `nrf52840_s140_v7.ld` was added to the installed Adafruit core from Seeed's
  Adafruit_nRF52_Arduino fork, along with the XIAO variant files).
- `rak4631` — RAK WisBlock RAK4631 (needs the RAKwireless WisBlock patch in
  `~/.platformio`, already applied on this machine).
- `feather_sx1276` — Adafruit Feather nRF52840 + external SX1276/RFM95.

## Usage

```bash
pio run -e xiao_a -t upload --upload-port /dev/ttyACM0    # flash node A
pio run -e xiao_b -t upload --upload-port /dev/ttyACM1    # flash node B
# then, in two separate terminals:
pio device monitor -p /dev/ttyACM0 -b 115200              # chat on node A
pio device monitor -p /dev/ttyACM1 -b 115200              # chat on node B
```

Type a line and press Enter — it appears on the other board as
`[LoRa <-] <text> (RSSI ... SNR ...)`. Keepalive slots print as `~` lines
about every 1.5 s and show the link is alive. Verified end-to-end with all
test messages delivered in both directions.
