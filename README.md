# XN297L Arduino Library

An Arduino library for the **XN297L** 2.4 GHz RF transceiver, designed for STM32 (Blue Pill / STM32duino) and other Arduino-compatible boards. The library uses the MCU's **hardware SPI** (default `SPI` or any custom `SPIClass` instance) and supports both **interrupt-driven** and **polling-based** operation.

---

## Table of Contents

1. [Features](#features)
2. [Hardware Requirements](#hardware-requirements)
3. [Wiring](#wiring)
4. [Installation](#installation)
5. [Library Structure](#library-structure)
6. [API Reference](#api-reference)
7. [Basic Usage](#basic-usage)
8. [Sender Example](#sender-example)
9. [Interrupt-Based Receiver Example](#interrupt-based-receiver-example)
10. [Polling-Based Receiver Example](#polling-based-receiver-example)
11. [Using a Custom SPI Instance](#using-a-custom-spi-instance)
12. [Important Notes & Limitations](#important-notes--limitations)
13. [Troubleshooting](#troubleshooting)

---

## Features

- ✅ Hardware SPI (default `SPI` or custom `SPIClass`)
- ✅ Assignable **CE**, **CSN**, and **IRQ** pins
- ✅ **Interrupt mode** (falling-edge IRQ) or **polling mode**
- ✅ TX and RX mode switching
- ✅ Configurable RF channel (0–125)
- ✅ Configurable address (default 5 bytes)
- ✅ Configurable payload size (up to 32 bytes)
- ✅ Register-level access based on the XN297L datasheet
- ✅ Compatible with STM32duino (STM32F1 Blue Pill tested)

---

## Hardware Requirements

| Component | Notes |
|-----------|-------|
| XN297L module | 3.3 V logic, QFN20 or SOP8 breakout |
| STM32 Blue Pill (STM32F103C8T6) | Or any Arduino-compatible board with hardware SPI |
| 10 µF + 0.1 µF capacitors | Decoupling on XN297L VDD |
| 2.4 GHz antenna | PCB antenna or 31 mm wire (λ/4) |
| ST-Link V2 | For programming the Blue Pill |

> ⚠️ **The XN297L is a 3.3 V device.** Do not connect 5 V logic directly.

---

## Wiring

### Default SPI (STM32 Blue Pill – SPI1)

| XN297L Pin | Blue Pill Pin | Function |
|------------|---------------|----------|
| VDD | 3.3V | Power (add 10 µF + 0.1 µF to GND) |
| VSS | GND | Ground |
| SCK | PA5 | SPI1 SCK |
| MOSI | PA7 | SPI1 MOSI |
| MISO | PA6 | SPI1 MISO |
| CSN | PA4 | Chip select (GPIO) |
| CE | PA0 | Chip enable (GPIO) |
| IRQ | PA1 | Interrupt (active low, optional) |

| STM32 Blue Pill | XN297L Module |
|------------|---------------|
| 3.3V | VDD (+ decoupling caps) |
| GND | VSS |
| PA5 (SPI1_SCK) | VSS |
| PA7 (SPI1_MOSI) | MOSI |
| PA6 (SPI1_MISO) | MISO |
| PA4 (GPIO) | CSN |
| PA0 (GPIO) | CE |
| PA1 (EXTI) | IRQ (optional, internal pull-up) |

> 💡 If you do not need interrupt mode, you can leave the **IRQ** pin unconnected (pass `255` as the IRQ pin to the constructor).

## Installation

### Option 1: Manual Installation

  1. Download or clone this repository.
  2. Copy the `XN297L` folder into your Arduino `libraries/` directory:
     - **Windows:** `Documents/Arduino/libraries/`
     - **Linux:** `~/Arduino/libraries/`
     - **macOS:** `~/Documents/Arduino/libraries/`
  3. Restart the Arduino IDE.
  4. Verify the library appears under **Sketch → Include Library → XN297L**.

### Option 2: ZIP Import

1. Zip the `XN297L` folder.
2. In Arduino IDE: **Sketch → Include Library → Add .ZIP Library…**
3. Select the ZIP file.

### Board Support (STM32)

  Install the **STM32duino** board package:
  
  1. Open **File → Preferences**.
  2. Add this URL to *Additional Boards Manager URLs*:
     -  **https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json**
  3. Go to **Tools → Board → Boards Manager**, search for **STM32**, and install **STM32 MCU based boards**.
  4. Select **Generic STM32F1 series → BluePill F103C8**.
  5. Set **Upload method: ST-Link**.

## Library Structure
  XN297L/.
  ├── src/
  │ ├── XN297L.h
  │ └── XN297L.cpp
  ├── examples/
  │ ├── Sender/
  │ │ └── Sender.ino
  │ ├── Receiver_IRQ/
  │ │ └── Receiver_IRQ.ino
  │ └── Receiver_Polling/
  │ └── Receiver_Polling.ino
  ├── library.properties
  └── README.md

## API Reference

### Constructor

    XN297L(uint8_t cePin, uint8_t csnPin, uint8_t irqPin = 255, SPIClass &spi = SPI);
      Parameter	Description
        cePin	GPIO connected to XN297L CE
        csnPin	GPIO connected to XN297L CSN (active low)
        irqPin	GPIO connected to XN297L IRQ. Use 255 to disable interrupt mode
        spi	Hardware SPI instance (default SPI)
      
    Methods
      Method	Description
      bool begin()	Initializes pins, SPI, and the XN297L. Returns true on success
      void setAddress(const uint8_t *addr, uint8_t len = 5)	Sets RX and TX address (default 5 bytes)
      void setChannel(uint8_t ch)	Sets RF channel (0–125)
      void setPayloadSize(uint8_t size)	Sets fixed payload size (1–32 bytes)
      bool txMode()	Switches the radio to TX mode
      bool send(const uint8_t *data, uint8_t len)	Sends a payload. Returns true on TX_DS, false on MAX_RT or timeout
      bool rxMode()	Switches the radio to RX mode
      bool available()	Returns true if an RX packet is ready (RX_DR set)
      bool read(uint8_t *buffer, uint8_t len)	Reads payload from RX FIFO. Returns true on success
      void enableIRQ()	Enables IRQ mode (call before begin() if using interrupts)
      
    Status Flags (Internal)
      Flag	Value	Meaning
        XN297L_STATUS_RX_DR	0x40	RX data ready
        XN297L_STATUS_TX_DS	0x20	TX data sent
        XN297L_STATUS_MAX_RT	0x10	Max retries reached
 
## Basic Usage

### 1. Create a radio object

    #include <XN297L.h>
    // CE = PA0, CSN = PA4, IRQ = PA1
    XN297L radio(PA0, PA4, PA1);
    
    For polling mode (no IRQ):
      XN297L radio(PA0, PA4, 255);
  
### 2. Initialize and configure

    void setup() {
        radio.begin();
        radio.setAddress((uint8_t[]){0xE7, 0xE7, 0xE7, 0xE7, 0xE7});
        radio.setChannel(76);
        radio.setPayloadSize(32);
    }

### 3. Send or receive

    TX: radio.txMode(); then radio.send(data, len);

    RX: radio.rxMode(); then poll radio.available() or use the IRQ pin

## Sender Example

    #include <XN297L.h>
    
    XN297L radio(PA0, PA4, 255);  // No IRQ for sender
    
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

## Interrupt-Based Receiver Example

The XN297L IRQ pin is active low. Attach a falling-edge interrupt to your chosen IRQ GPIO. Because STM32duino attachInterrupt does not accept member function pointers, set a global volatile bool flag in the ISR and handle the SPI read in loop().

    #include <XN297L.h>
    
    XN297L radio(PA0, PA4, PA1);  // IRQ = PA1
    
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
    
        pinMode(PA1, INPUT_PULLUP);
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
  
    ⚠️ Keep ISR short. Do not perform SPI transfers inside the ISR. Use a flag and read in loop().

## Polling-Based Receiver Example

If you do not connect the IRQ pin, poll available() in the main loop:

    #include <XN297L.h>
    
    XN297L radio(PA0, PA4, 255);  // No IRQ
    
    uint8_t address[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
    
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
    
        Serial.println("Polling Receiver ready");
    }
    
    void loop() {
        if (radio.available()) {
            uint8_t buffer[32];
            if (radio.read(buffer, 32)) {
                Serial.print("RX: ");
                Serial.println((char*)buffer + 1);
            }
        }
    }

## Using a Custom SPI Instance

By default, the library uses the global SPI object. To use a different hardware SPI bus (e.g., SPI2 on STM32), pass a SPIClass instance to the constructor.

  STM32 SPI2 Example
 
    #include <XN297L.h>
    
    // SPI2 on Blue Pill: SCK=PB13, MISO=PB14, MOSI=PB15
    SPIClass SPI_2(PB15, PB14, PB13);
    
    // CE=PA0, CSN=PA4, IRQ=PA1, using SPI_2
    XN297L radio(PA0, PA4, PA1, SPI_2);
  
Notes

    The CSN pin is software-controlled and is not the hardware NSS pin.

    Make sure the SPI instance is initialized before begin() is called — begin() calls _spi.begin() internally.

    Supported SPI modes: SPI Mode 0 (CPOL=0, CPHA=0). The XN297L supports up to 4 Mbps SPI, but 1–2 MHz is recommended for reliable operation.

## Important Notes & Limitations
1. Register Map and Datasheet

The register addresses in this library are based on the XN297L datasheet, not the nRF24L01+ datasheet. While many registers are similar, the XN297L uses a different preamble and has some differing configuration bits.
2. Auto-ACK and CRC

    Auto-ACK is disabled (EN_AA = 0x00) by default to simplify the implementation.

    The XN297L supports CRC, but this library does not expose CRC configuration in the public API. For reliable links, implement application-layer acknowledgment in your sketch.

3. Payload Size

    Fixed payload size, configured via setPayloadSize().

    Maximum 32 bytes per packet.

    Both TX and RX must use the same payload size.

4. Channel and Address Matching

    TX and RX must use the same channel (setChannel) and same address (setAddress).

    Default channel: 76 (2.476 GHz).

    Default address: {0xE7, 0xE7, 0xE7, 0xE7, 0xE7}.

5. IRQ Pin

    The IRQ pin is active low and maskable.

    In interrupt mode, always clear the RX_DR flag after reading the payload (the library does this in read()).

    If you see spurious interrupts, add a 10 kΩ pull-up + 100 nF to GND RC filter on the IRQ line.

6. SPI Speed

    The XN297L supports up to 4 Mbps SPI.

    In power-down/standby mode, the maximum SPI clock is 1 Mbps.

    The library uses the default SPI clock. If you experience instability, reduce the SPI clock divider.

7. No nRF24L01+ Compatibility

    This library is not a drop-in replacement for nRF24L01+ libraries.

    If you need nRF24-style features (auto-ACK, dynamic payloads, CRC configuration), extend the library using the XN297L datasheet.

## Troubleshooting
Symptom	Possible Cause	Fix
begin() never returns / hangs	SPI wiring or power issue	Check 3.3 V supply and decoupling caps; verify SCK/MOSI/MISO
send() always returns false	TX and RX channel/address mismatch	Ensure both sides use the same setChannel() and setAddress()
No packets received	IRQ not connected in IRQ mode	Either connect IRQ or use polling mode (irqPin = 255)
Garbage data received	Payload size mismatch	Set the same setPayloadSize() on TX and RX
Spurious interrupts	Noisy IRQ line	Add 10 kΩ pull-up + 100 nF to GND on IRQ
SPI communication fails	Wrong SPI mode	Ensure SPI Mode 0 (CPOL=0, CPHA=0)
Works at short range only	Poor antenna / no matching	Use a proper 2.4 GHz antenna with 50 Ω matching
License

This library is provided as-is for educational and prototyping purposes. Refer to the XN297L datasheet for production-level register configuration and RF certification requirements.
References

    XN297L datasheet (PANCHIP) – register map, SPI timing, IRQ behavior

    STM32duino board support: https://github.com/stm32duino/Arduino_Core_STM32

    Arduino SPI library: https://www.arduino.cc/reference/en/language/functions/communication/spi/

This README covers installation, wiring, API usage, sender/receiver examples (both IRQ and polling modes), custom SPI usage, and troubleshooting. Save it as `README.md` in the root of the `XN297L` library folder.

