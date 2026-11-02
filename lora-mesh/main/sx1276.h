#pragma once

#include "spi_driver.h"
#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

// ─── Registradores SX1276 ───────────────────────────────────────────────────
#define SX_REG_FIFO           0x00
#define SX_REG_OP_MODE        0x01
#define SX_REG_FR_MSB         0x06
#define SX_REG_FR_MID         0x07
#define SX_REG_FR_LSB         0x08
#define SX_REG_PA_CONFIG      0x09
#define SX_REG_FIFO_ADDR_PTR  0x0D
#define SX_REG_FIFO_TX_BASE   0x0E
#define SX_REG_FIFO_RX_BASE   0x0F
#define SX_REG_RX_CURR_ADDR   0x10
#define SX_REG_IRQ_FLAGS      0x12
#define SX_REG_RX_NB_BYTES    0x13
#define SX_REG_PKT_RSSI       0x1A
#define SX_REG_MODEM_CFG1     0x1D
#define SX_REG_MODEM_CFG2     0x1E
#define SX_REG_SYMB_TIMEOUT   0x1F
#define SX_REG_PAYLOAD_LEN    0x22
#define SX_REG_MODEM_CFG3     0x26
#define SX_REG_RSSI_WIDEBAND  0x2C
#define SX_REG_DETECTION_OPT  0x31
#define SX_REG_DETECTION_THR  0x37
#define SX_REG_SYNC_WORD      0x39
#define SX_REG_DIO_MAPPING1   0x40
#define SX_REG_VERSION        0x42

// ─── Modos de operação ───────────────────────────────────────────────────────
#define SX_MODE_SLEEP         0x00
#define SX_MODE_STDBY         0x01
#define SX_MODE_TX            0x03
#define SX_MODE_RX_CONT       0x05
#define SX_MODE_LORA          0x80  // bit 7 = LoRa (vs FSK)

// ─── IRQ flags (RegIrqFlags) ─────────────────────────────────────────────────
#define SX_IRQ_TX_DONE        0x08
#define SX_IRQ_RX_DONE        0x40
#define SX_IRQ_CRC_ERROR      0x20

// ─── Frequência 433 MHz ──────────────────────────────────────────────────────
// Frf = Freq * 2^19 / 32e6
// 433e6 * 524288 / 32e6 = 7094272 = 0x6C8000
#define LORA_FREQ_MSB         0x6C
#define LORA_FREQ_MID         0x80
#define LORA_FREQ_LSB         0x00

// ─── API pública ─────────────────────────────────────────────────────────────
esp_err_t sx1276_init(spi_device_handle_t handle);
esp_err_t sx1276_send(spi_device_handle_t handle, const uint8_t *data, size_t len);
int       sx1276_receive(spi_device_handle_t handle, uint8_t *buf, size_t max_len, int8_t *rssi);
uint8_t   sx1276_version(spi_device_handle_t handle);
