/*
 * LoRa serial chat between two boards, riding on a continuous
 * ping-pong link (the traffic pattern proven reliable on this
 * hardware — long idle receive windows make the SX1262 go deaf).
 *
 * Node A initiates a round trip every ~1.5 s; node B replies to every
 * packet. Packets carry either a keepalive ("~", not displayed) or a
 * line typed into the serial monitor. Typed lines are queued and go
 * out with the next slot; received lines print as [LoRa <-].
 *
 * Flash xiao_a to one board and xiao_b to the other, open a serial
 * terminal on each, and type.
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

#define MAX_MSG_LEN 60

static char lineBuf[MAX_MSG_LEN + 1];
static size_t lineLen = 0;
static char outMsg[MAX_MSG_LEN + 1] = "~";
static bool outPending = false;

/* collect typed characters; a completed line is queued for sending */
static void pollSerial(void) {
    while (Serial.available() > 0) {
        char c = (char)Serial.read();
        if (c == '\n' || c == '\r') {
            if (lineLen > 0 && !outPending) {
                lineBuf[lineLen] = '\0';
                memcpy(outMsg, lineBuf, lineLen + 1);
                outPending = true;
                lineLen = 0;
            }
        } else if (lineLen < MAX_MSG_LEN) {
            lineBuf[lineLen++] = c;
        }
    }
}

static void sendHi(void) {
    const char *msg = outPending ? outMsg : "~";
    int state = radio.transmit((uint8_t *)msg, strlen(msg));
    if (state == RADIOLIB_ERR_NONE) {
        Serial.print("[LoRa ->] ");
        Serial.println(msg);
        outPending = false;
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
            Serial.print("[LoRa <-] ");
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

    Serial.println("LoRa chat ready, node " NODE_NAME
                   ". Type a message and press Enter to send.");
}

#if defined(NODE_A)
/* Node A: initiator — say hi, wait for B's reply, repeat every second */
void loop() {
    pollSerial();
    sendHi();
    listenFor(3000);
    uint32_t start = millis();
    while (millis() - start < 500) {
        pollSerial();
    }
}
#else
/* Node B: responder — wait for a hi, then reply with our own */
void loop() {
    pollSerial();
    if (listenFor(10000)) {
        /* give A time to switch to receive mode */
        uint32_t start = millis();
        while (millis() - start < 100) {
            pollSerial();
        }
        sendHi();
    }
}
#endif
