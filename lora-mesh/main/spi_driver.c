#include "spi_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "SPI";

// Inicializa o barramento SPI e adiciona o SX1276 como device
esp_err_t spi_driver_init(spi_device_handle_t *handle)
{
    // Reset via GPIO antes de qualquer comunicação
    gpio_set_direction(PIN_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Configura o barramento SPI
    spi_bus_config_t bus = {
        .mosi_io_num     = PIN_MOSI,
        .miso_io_num     = PIN_MISO,
        .sclk_io_num     = PIN_SCK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = 256,
    };

    esp_err_t ret = spi_bus_initialize(SPI_HOST, &bus, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao inicializar barramento SPI: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configura o device SX1276
    // SX1276 usa modo SPI 0 (CPOL=0, CPHA=0)
    spi_device_interface_config_t dev = {
        .clock_speed_hz = SPI_FREQ_HZ,
        .mode           = 0,
        .spics_io_num   = PIN_CS,
        .queue_size     = 8,
    };

    ret = spi_bus_add_device(SPI_HOST, &dev, handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao adicionar device SPI: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "SPI inicializado — SCK:%d MOSI:%d MISO:%d CS:%d RST:%d",
             PIN_SCK, PIN_MOSI, PIN_MISO, PIN_CS, PIN_RST);
    return ESP_OK;
}

// Lê 1 registrador do SX1276
// Protocolo: byte de endereço com bit7=0 (leitura), depois 1 byte de resposta
uint8_t spi_read_reg(spi_device_handle_t handle, uint8_t reg)
{
    uint8_t tx[2] = { reg & 0x7F, 0x00 };  // bit7=0 → leitura
    uint8_t rx[2] = { 0 };

    spi_transaction_t t = {
        .length    = 16,  // bits
        .tx_buffer = tx,
        .rx_buffer = rx,
    };

    spi_device_transmit(handle, &t);
    return rx[1];
}

// Escreve 1 registrador no SX1276
// Protocolo: byte de endereço com bit7=1 (escrita), depois valor
void spi_write_reg(spi_device_handle_t handle, uint8_t reg, uint8_t val)
{
    uint8_t tx[2] = { reg | 0x80, val };  // bit7=1 → escrita

    spi_transaction_t t = {
        .length    = 16,
        .tx_buffer = tx,
        .rx_buffer = NULL,
    };

    spi_device_transmit(handle, &t);
}

// Lê múltiplos bytes (burst read) — usado pra ler o FIFO
void spi_read_burst(spi_device_handle_t handle, uint8_t reg, uint8_t *buf, size_t len)
{
    uint8_t addr = reg & 0x7F;

    spi_transaction_t t = {
        .length    = 8 * (len + 1),
        .tx_buffer = &addr,
        .rx_buffer = buf,
    };

    spi_device_transmit(handle, &t);
}

// Escreve múltiplos bytes (burst write) — usado pra escrever no FIFO
void spi_write_burst(spi_device_handle_t handle, uint8_t reg, const uint8_t *buf, size_t len)
{
    uint8_t tx[len + 1];
    tx[0] = reg | 0x80;
    memcpy(&tx[1], buf, len);

    spi_transaction_t t = {
        .length    = 8 * (len + 1),
        .tx_buffer = tx,
        .rx_buffer = NULL,
    };

    spi_device_transmit(handle, &t);
}
