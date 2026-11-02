#pragma once

#include "driver/spi_master.h"
#include "driver/gpio.h"

// Pinagem ESP32 + XL1278-SMT
#define PIN_SCK     18
#define PIN_MOSI    23
#define PIN_MISO    19
#define PIN_CS      5
#define PIN_RST     14
#define PIN_DIO0    26
// DIO1 opcional — só se precisar de interrupção de RX timeout
// #define PIN_DIO1  33

// Host SPI (SPI2 = HSPI)
#define SPI_HOST    SPI2_HOST
#define SPI_FREQ_HZ 8000000  // 8 MHz

esp_err_t spi_driver_init(spi_device_handle_t *handle);
uint8_t   spi_read_reg(spi_device_handle_t handle, uint8_t reg);
void      spi_write_reg(spi_device_handle_t handle, uint8_t reg, uint8_t val);
void      spi_read_burst(spi_device_handle_t handle, uint8_t reg, uint8_t *buf, size_t len);
void      spi_write_burst(spi_device_handle_t handle, uint8_t reg, const uint8_t *buf, size_t len);
