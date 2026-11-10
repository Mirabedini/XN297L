#include <XN297L.h>

// Blue Pill default SPI: PA5(SCK), PA6(MISO), PA7(MOSI)
// CE = PA0, CSN = PA4, IRQ not used for sender
XN297L radio(PA0, PA4, 255);

uint8_t address[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
uint8_t counter = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);

    if (!radio.begin()) {
        Serial.println("XN297L init failed");
        while (1);
    }

    radio.setAddress(address);
    radio.setChannel(76);
    radio.txMode();

    Serial.println("Sender ready");
}

void loop() {
    uint8_t payload[32] = {0};
    payload[0] = counter++;
    snprintf((char*)payload + 1, 31, "Hello XN297L");

    if (radio.send(payload, 32)) {
        Serial.print("Sent: ");
        Serial.println((char*)payload + 1);
    } else {
        Serial.println("Send failed");
    }

    delay(1000);
}
