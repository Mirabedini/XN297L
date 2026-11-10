#include "XN297L.h"

XN297L::XN297L(uint8_t cePin, uint8_t csnPin, uint8_t irqPin, SPIClass &spi)
    : _ce(cePin), _csn(csnPin), _irq(irqPin), _spi(spi), _useIRQ(false), _irqFlag(false) {}

bool XN297L::begin() {
    pinMode(_ce, OUTPUT);
    pinMode(_csn, OUTPUT);
    digitalWrite(_ce, LOW);
    digitalWrite(_csn, HIGH);

    _spi.begin();

    // Enter power down, then configure
    writeReg(XN297L_REG_CONFIG, 0x0C);  // CRC enabled, PWR_UP=1, PRIM_RX=1
    delay(5);

    // Basic configuration: disable auto-ACK, fixed payload, 5-byte address
    writeReg(XN297L_REG_EN_AA, 0x00);
    writeReg(XN297L_REG_EN_RXADDR, 0x01);
    writeReg(XN297L_REG_SETUP_AW, 0x03);  // 5-byte address
    writeReg(XN297L_REG_RF_SETUP, 0x06);  // 1Mbps, 0dBm
    writeReg(XN297L_REG_RX_PW_P0, 32);    // Fixed 32-byte payload

    flushTX();
    flushRX();
    clearStatus();

    // Configure interrupt pin
    if (_irq != 255) {
        pinMode(_irq, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(_irq), []() {
            // Static ISR - flag handled in instance via wrapper
        }, FALLING);
        // Use STM32 attachInterrupt with parameter (if supported)
        // For portability, use global flag approach below
    }

    return true;
}

void XN297L::setAddress(const uint8_t *addr, uint8_t len) {
    writeBuf(XN297L_REG_RX_ADDR_P0, addr, len);
    writeBuf(XN297L_REG_TX_ADDR, addr, len);
}

void XN297L::setChannel(uint8_t ch) {
    if (ch > 125) ch = 125;
    writeReg(XN297L_REG_RF_CH, ch);
}

void XN297L::setPayloadSize(uint8_t size) {
    if (size > 32) size = 32;
    writeReg(XN297L_REG_RX_PW_P0, size);
}

bool XN297L::txMode() {
    digitalWrite(_ce, LOW);
    uint8_t cfg = readReg(XN297L_REG_CONFIG);
    cfg &= ~0x01;  // PRIM_RX = 0
    cfg |= 0x02;   // PWR_UP = 1
    writeReg(XN297L_REG_CONFIG, cfg);
    delayMicroseconds(150);
    return true;
}

bool XN297L::send(const uint8_t *data, uint8_t len) {
    digitalWrite(_ce, LOW);
    flushTX();
    clearStatus();

    csnLow();
    _spi.transfer(XN297L_CMD_W_TX_PLOAD);
    for (uint8_t i = 0; i < len; i++) _spi.transfer(data[i]);
    csnHigh();

    // Pulse CE to trigger transmission
    digitalWrite(_ce, HIGH);
    delayMicroseconds(20);
    digitalWrite(_ce, LOW);

    // Wait for TX_DS or MAX_RT
    uint32_t start = millis();
    while (millis() - start < 10) {
        uint8_t status = readReg(XN297L_REG_STATUS);
        if (status & XN297L_STATUS_TX_DS) {
            writeReg(XN297L_REG_STATUS, XN297L_STATUS_TX_DS);
            return true;
        }
        if (status & XN297L_STATUS_MAX_RT) {
            writeReg(XN297L_REG_STATUS, XN297L_STATUS_MAX_RT);
            flushTX();
            return false;
        }
    }
    return false;
}

bool XN297L::rxMode() {
    digitalWrite(_ce, LOW);
    uint8_t cfg = readReg(XN297L_REG_CONFIG);
    cfg |= 0x01;   // PRIM_RX = 1
    cfg |= 0x02;   // PWR_UP = 1
    writeReg(XN297L_REG_CONFIG, cfg);
    flushRX();
    clearStatus();
    digitalWrite(_ce, HIGH);
    delayMicroseconds(150);
    return true;
}

bool XN297L::available() {
    uint8_t status = readReg(XN297L_REG_STATUS);
    return (status & XN297L_STATUS_RX_DR) != 0;
}

bool XN297L::read(uint8_t *buffer, uint8_t len) {
    if (!available()) return false;

    csnLow();
    _spi.transfer(XN297L_CMD_R_RX_PLOAD);
    for (uint8_t i = 0; i < len; i++) buffer[i] = _spi.transfer(0xFF);
    csnHigh();

    writeReg(XN297L_REG_STATUS, XN297L_STATUS_RX_DR);
    return true;
}

void XN297L::enableIRQ() {
    _useIRQ = true;
}

// Low-level helpers
void XN297L::csnLow()  { digitalWrite(_csn, LOW); }
void XN297L::csnHigh() { digitalWrite(_csn, HIGH); }

void XN297L::writeReg(uint8_t reg, uint8_t val) {
    csnLow();
    _spi.transfer(XN297L_CMD_W_REG | (reg & 0x1F));
    _spi.transfer(val);
    csnHigh();
}

uint8_t XN297L::readReg(uint8_t reg) {
    csnLow();
    _spi.transfer(XN297L_CMD_R_REG | (reg & 0x1F));
    uint8_t val = _spi.transfer(0xFF);
    csnHigh();
    return val;
}

void XN297L::writeBuf(uint8_t reg, const uint8_t *buf, uint8_t len) {
    csnLow();
    _spi.transfer(XN297L_CMD_W_REG | (reg & 0x1F));
    for (uint8_t i = 0; i < len; i++) _spi.transfer(buf[i]);
    csnHigh();
}

void XN297L::readBuf(uint8_t reg, uint8_t *buf, uint8_t len) {
    csnLow();
    _spi.transfer(XN297L_CMD_R_REG | (reg & 0x1F));
    for (uint8_t i = 0; i < len; i++) buf[i] = _spi.transfer(0xFF);
    csnHigh();
}

void XN297L::flushTX() { csnLow(); _spi.transfer(XN297L_CMD_FLUSH_TX); csnHigh(); }
void XN297L::flushRX() { csnLow(); _spi.transfer(XN297L_CMD_FLUSH_RX); csnHigh(); }

void XN297L::clearStatus() {
    writeReg(XN297L_REG_STATUS, XN297L_STATUS_RX_DR | XN297L_STATUS_TX_DS | XN297L_STATUS_MAX_RT);
}
