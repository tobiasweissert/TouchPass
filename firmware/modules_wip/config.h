// TouchPass Configuration
// Pin definitions and constants

#ifndef TOUCHPASS_CONFIG_H
#define TOUCHPASS_CONFIG_H

#include "sdkconfig.h"

// ===== Pin Configuration =====
#if CONFIG_IDF_TARGET_ESP32S3
  // ESP32-S3: D4=GPIO5, D5=GPIO6 (Seeed XIAO silkscreen labels)
  #define FP_TX_PIN 5   // D4 - Sensor TX
  #define FP_RX_PIN 6   // D5 - Sensor RX
  // USB Serial uses UART0 (GPIO43/44) automatically
#else
  // ESP32-C6: Standard pinout
  #define FP_TX_PIN 16  // D6
  #define FP_RX_PIN 17  // D7
#endif

// ===== Fingerprint Sensor Protocol =====
#define FP_HEADER 0xEF01
#define FP_DEFAULT_ADDR 0xFFFFFFFF
#define FP_CMD_PACKET 0x01
#define FP_BAUD_RATE 57600

// Sensor Commands
#define CMD_GENIMG 0x01
#define CMD_IMG2TZ 0x02
#define CMD_SEARCH 0x04
#define CMD_REGMODEL 0x05
#define CMD_STORE 0x06
#define CMD_DELETCHAR 0x0C
#define CMD_EMPTY 0x0D
#define CMD_READSYSPARA 0x0F
#define CMD_TEMPLATENUM 0x1D
#define CMD_READINDEXTABLE 0x1F
#define CMD_AURALEDCONFIG 0x35
#define CMD_CHECKSENSOR 0x36
#define CMD_HANDSHAKE 0x40

// LED Control
#define LED_RED 0x01
#define LED_BLUE 0x02
#define LED_GREEN 0x04
#define LED_CYAN 0x06
#define LED_BREATHING 0x01
#define LED_FLASHING 0x02
#define LED_ON 0x03
#define LED_OFF 0x04

// ===== WiFi Configuration =====
#define AP_SSID "TouchPass"
#define AP_PASSWORD "touchpass"

// ===== Timing Constants =====
#define LONG_TOUCH_MS 5000
#define SENSOR_TIMEOUT_MS 3000
#define ENROLL_TIMEOUT_MS 60000

// ===== Serial Configuration =====
#define CONFIG_BAUD_RATE 115200
#define USB_WAIT_TIME_MS 3000

#endif // TOUCHPASS_CONFIG_H
