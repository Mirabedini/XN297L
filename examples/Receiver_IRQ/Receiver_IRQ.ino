#include <XN297L.h>

// CE = PA0, CSN = PA4, IRQ = PA1
XN297L radio(PA0, PA4, PA1);

uint8_t address[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
volatile bool packetReady = false;

void setup() {
    Serial.begin(115200);
    delay(1000);

    if (!radio.begin()) {
        Serial.println("XN297L init failed");
        while (1);
    }

    radio.setAddress(address);
    radio.setChannel(76);
    radio.rxMode();

    // Attach interrupt: XN297L IRQ is active-low, falling edge triggers
    attachInterrupt(digitalPinToInterrupt(PA1), []() {
        packetReady = true;
    }, FALLING);

    Serial.println("IRQ Receiver ready");
}

void loop() {
    if (packetReady) {
        packetReady = false;

        uint8_t buffer[32];
        if (radio.read(buffer, 32)) {
            Serial.print("RX: ");
            Serial.println((char*)buffer + 1);
        }
    }
}
