/*
 * Bidirectional LoRa transceiver ("hi" ping-pong) for two boards.
 *
 * Node A: sends "hi from A" every second, then listens for the reply.
 * Node B: listens, prints anything received, replies "hi from B".
 * Both boards therefore transmit AND receive.
 *
 * Radio parameters (865.1 MHz, BW 125 kHz, SF10, CR 4/5, preamble 8,
 * private sync word, +4 dBm) match Zephyr's samples/drivers/lora samples.
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

#if defined(NODE_A)
#define NODE_NAME "A"
#else
#define NODE_NAME "B"
#endif

static const char hi_msg[] = "hi from " NODE_NAME;

static void sendHi(void) {
    int state = radio.transmit((uint8_t *)hi_msg, sizeof(hi_msg) - 1);
    if (state == RADIOLIB_ERR_NONE) {
        Serial.print("Sent: ");
        Serial.println(hi_msg);
    } else {
        Serial.print("Send failed, code ");
        Serial.println(state);
    }
}

/* Listen up to timeout_ms; print and return true if a packet arrived. */
static bool listenFor(uint32_t timeout_ms) {
    uint32_t start = millis();
    uint8_t buf[64];

    while (millis() - start < timeout_ms) {
        int state = radio.receive(buf, sizeof(buf));
        if (state == RADIOLIB_ERR_NONE) {
            size_t len = radio.getPacketLength();
            Serial.print("Received: ");
            Serial.write(buf, len);
            Serial.print("  (RSSI ");
            Serial.print(radio.getRSSI());
            Serial.print(" dBm, SNR ");
            Serial.print(radio.getSNR());
            Serial.println(" dB)");
            return true;
        }
        if (state != RADIOLIB_ERR_RX_TIMEOUT) {
            Serial.print("Receive failed, code ");
            Serial.println(state);
        }
    }
    return false;
}

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
    /* SX1262 modules drive the RF switch from DIO2 */
    radio.setDio2AsRfSwitch(true);
#endif

    Serial.println("LoRa transceiver ready, node " NODE_NAME);
}

#if defined(NODE_A)
/* Node A: initiator — say hi, wait for B's reply, repeat every second */
void loop() {
    sendHi();
    if (!listenFor(3000)) {
        Serial.println("No reply from B");
    }
    delay(1000);
}
#else
/* Node B: responder — wait for a hi, then reply with our own */
void loop() {
    if (listenFor(60000)) {
        /* give A time to switch to receive mode */
        delay(100);
        sendHi();
    }
}
#endif
