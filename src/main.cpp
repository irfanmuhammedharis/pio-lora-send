/*
 * PlatformIO / Arduino port of Zephyr samples/drivers/lora/send
 *
 * Radio parameters match the Zephyr sample exactly, so packets are
 * receivable by the Zephyr samples/drivers/lora/receive sample:
 *   865.1 MHz, BW 125 kHz, SF10, CR 4/5, preamble 8,
 *   private (non-public) sync word, +4 dBm TX power.
 */

#include <Arduino.h>
#include <RadioLib.h>

#if defined(USE_SX1262)
/* NSS, DIO1, NRST, BUSY */
SX1262 radio = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY);
#elif defined(USE_SX1276)
/* NSS, DIO0, NRST, DIO1 */
SX1276 radio = new Module(LORA_CS, LORA_DIO0, LORA_RST, LORA_DIO1);
#else
#error "Define USE_SX1262 or USE_SX1276 in build_flags"
#endif

#define MAX_DATA_LEN 12

static uint8_t data[MAX_DATA_LEN] = {'h', 'e', 'l', 'l', 'o', 'w',
                                     'o', 'r', 'l', 'd', ' ', '0'};

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000) {
    }

    /* freq MHz, BW kHz, SF, CR denominator, sync word, power dBm, preamble */
#if defined(USE_SX1262)
    /* Wio-SX1262: TCXO powered from DIO3 at 1.8 V
     * (per Meshtastic seeed_xiao_nrf52840_kit variant) */
    int state = radio.begin(865.1, 125.0, 10, 5,
                            RADIOLIB_SX126X_SYNC_WORD_PRIVATE, 4, 8, 1.8);
#else
    int state = radio.begin(865.1, 125.0, 10, 5,
                            RADIOLIB_SX127X_SYNC_WORD, 4, 8);
#endif
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print("LoRa config failed, code ");
        Serial.println(state);
        while (true) {
            delay(1000);
        }
    }

#if defined(USE_SX1262)
    /* SX1262 modules (incl. RAK4631) drive the RF switch from DIO2 */
    radio.setDio2AsRfSwitch(true);
#endif

    Serial.println("LoRa radio configured");
}

#if defined(ROLE_RX)
/* Receiver: mirrors Zephyr samples/drivers/lora/receive */
void loop() {
    uint8_t buf[64];
    int state = radio.receive(buf, sizeof(buf));
    if (state == RADIOLIB_ERR_NONE) {
        size_t len = radio.getPacketLength();
        Serial.print("Received ");
        Serial.print(len);
        Serial.print(" bytes: ");
        Serial.write(buf, len);
        Serial.print("  RSSI ");
        Serial.print(radio.getRSSI());
        Serial.print(" dBm, SNR ");
        Serial.print(radio.getSNR());
        Serial.println(" dB");
    } else if (state != RADIOLIB_ERR_RX_TIMEOUT) {
        Serial.print("LoRa receive failed, code ");
        Serial.println(state);
    }
}
#else
void loop() {
    int state = radio.transmit(data, MAX_DATA_LEN);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print("LoRa send failed, code ");
        Serial.println(state);
        return;
    }

    Serial.print("Data sent ");
    Serial.print((char)data[MAX_DATA_LEN - 1]);
    Serial.println("!");

    /* Send data at 1s interval */
    delay(1000);

    /* Increment final character to differentiate packets */
    if (data[MAX_DATA_LEN - 1] == '9') {
        data[MAX_DATA_LEN - 1] = '0';
    } else {
        data[MAX_DATA_LEN - 1] += 1;
    }
}
#endif /* ROLE_RX */
