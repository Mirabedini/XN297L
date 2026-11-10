#ifndef XN297L_H
#define XN297L_H

#include <Arduino.h>
#include <SPI.h>

// XN297L Register Addresses (based on official datasheet)
#define XN297L_REG_CONFIG      0x00
#define XN297L_REG_EN_AA       0x01
#define XN297L_REG_EN_RXADDR   0x02
#define XN297L_REG_SETUP_AW    0x03
#define XN297L_REG_RF_CH       0x05
#define XN297L_REG_RF_SETUP    0x06
#define XN297L_REG_STATUS      0x07
#define XN297L_REG_RX_ADDR_P0  0x0A
#define XN297L_REG_TX_ADDR     0x10
#define XN297L_REG_RX_PW_P0    0x11
#define XN297L_REG_FIFO_STATUS 0x17

// SPI Commands
#define XN297L_CMD_R_REG       0x00  // 000AAAAA
#define XN297L_CMD_W_REG       0x20  // 001AAAAA
#define XN297L_CMD_R_RX_PLOAD  0x61
#define XN297L_CMD_W_TX_PLOAD  0xA0
#define XN297L_CMD_FLUSH_TX    0xE1
#define XN297L_CMD_FLUSH_RX    0xE2
#define XN297L_CMD_NOP         0xFF

// Status flags
#define XN297L_STATUS_RX_DR    0x40
#define XN297L_STATUS_TX_DS    0x20
#define XN297L_STATUS_MAX_RT   0x10

class XN297L {
public:
    // Constructor: default uses hardware SPI, CE/CSN/IRQ pins specified by user
    // irqPin = 255 means no interrupt (polling mode)
    XN297L(uint8_t cePin, uint8_t csnPin, uint8_t irqPin = 255, SPIClass &spi = SPI);

    bool begin();
    void setAddress(const uint8_t *addr, uint8_t len = 5);
    void setChannel(uint8_t ch);
    void setPayloadSize(uint8_t size);

    // TX mode
    bool txMode();
    bool send(const uint8_t *data, uint8_t len);

    // RX mode
    bool rxMode();
    bool available();
    bool read(uint8_t *buffer, uint8_t len);

    // IRQ mode configuration (call before begin if using interrupts)
    void enableIRQ();

private:
    uint8_t _ce, _csn, _irq;
    SPIClass &_spi;
    bool _useIRQ;
    volatile bool _irqFlag;

    void csnLow();
    void csnHigh();
    void writeReg(uint8_t reg, uint8_t val);
    uint8_t readReg(uint8_t reg);
    void writeBuf(uint8_t reg, const uint8_t *buf, uint8_t len);
    void readBuf(uint8_t reg, uint8_t *buf, uint8_t len);
    void flushTX();
    void flushRX();
    void clearStatus();

    static void IRQHandler(XN297L *instance);
};

#endif
