#include "sx1276.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "SX1276";

// Coloca o rádio num modo de operação
static void set_mode(spi_device_handle_t h, uint8_t mode)
{
    spi_write_reg(h, SX_REG_OP_MODE, SX_MODE_LORA | mode);
    vTaskDelay(pdMS_TO_TICKS(10));
}

// Inicializa o SX1276 em modo LoRa 433 MHz
esp_err_t sx1276_init(spi_device_handle_t handle)
{
    // Verifica se o chip responde (versão deve ser 0x12)
    uint8_t ver = sx1276_version(handle);
    if (ver != 0x12) {
        ESP_LOGE(TAG, "SX1276 nao encontrado! Version=0x%02X (esperado 0x12)", ver);
        ESP_LOGE(TAG, "Verifique a pinagem e a solda.");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "SX1276 detectado. Version=0x%02X", ver);

    // Sleep para poder configurar
    set_mode(handle, SX_MODE_SLEEP);

    // Define frequência: 433 MHz
    spi_write_reg(handle, SX_REG_FR_MSB, LORA_FREQ_MSB);
    spi_write_reg(handle, SX_REG_FR_MID, LORA_FREQ_MID);
    spi_write_reg(handle, SX_REG_FR_LSB, LORA_FREQ_LSB);

    // Base address do FIFO para TX e RX
    spi_write_reg(handle, SX_REG_FIFO_TX_BASE, 0x00);
    spi_write_reg(handle, SX_REG_FIFO_RX_BASE, 0x00);

    // ModemConfig1: BW=125kHz (0x7), CR=4/5 (0x1), implicit header off
    // bits[7:4]=BW, bits[3:1]=CR, bit[0]=implicit header
    // 0111 001 0 = 0x72
    spi_write_reg(handle, SX_REG_MODEM_CFG1, 0x72);

    // ModemConfig2: SF=7 (0x7), TX modo normal, CRC on
    // bits[7:4]=SF, bit[3]=TX mode, bit[2]=CRC en, bits[1:0]=sym timeout MSB
    // 0111 0 1 00 = 0x74
    spi_write_reg(handle, SX_REG_MODEM_CFG2, 0x74);

    // ModemConfig3: LNA gain auto, LowDataRateOptimize off
    spi_write_reg(handle, SX_REG_MODEM_CFG3, 0x04);

    // Potência de transmissão: PA_BOOST, +17dBm
    // bit7=PA_BOOST, bits[3:0]=OutputPower → 0x8F = max power
    spi_write_reg(handle, SX_REG_PA_CONFIG, 0x8F);

    // Sync word: 0x12 = rede privada (Meshtastic usa 0x2B; aqui usamos privado)
    spi_write_reg(handle, SX_REG_SYNC_WORD, 0x12);

    // Detection optimize e threshold para SF7-SF12
    spi_write_reg(handle, SX_REG_DETECTION_OPT, 0xC3);
    spi_write_reg(handle, SX_REG_DETECTION_THR, 0x0A);

    // DIO0 mapeado para RxDone em RX, TxDone em TX
    spi_write_reg(handle, SX_REG_DIO_MAPPING1, 0x00);

    // Standby — pronto para TX ou RX
    set_mode(handle, SX_MODE_STDBY);

    ESP_LOGI(TAG, "SX1276 configurado: 433MHz SF7 BW125 CR4/5 CRC:on");
    return ESP_OK;
}

// Transmite um pacote LoRa
// Retorna ESP_OK se TX done recebido dentro do timeout
esp_err_t sx1276_send(spi_device_handle_t handle, const uint8_t *data, size_t len)
{
    if (len > 255) return ESP_ERR_INVALID_SIZE;

    // Standby para poder escrever no FIFO
    set_mode(handle, SX_MODE_STDBY);

    // Aponta o ponteiro do FIFO para a base de TX
    spi_write_reg(handle, SX_REG_FIFO_ADDR_PTR, 0x00);

    // Escreve os dados no FIFO
    for (size_t i = 0; i < len; i++) {
        spi_write_reg(handle, SX_REG_FIFO, data[i]);
    }

    // Define o tamanho do payload
    spi_write_reg(handle, SX_REG_PAYLOAD_LEN, (uint8_t)len);

    // DIO0 = TxDone
    spi_write_reg(handle, SX_REG_DIO_MAPPING1, 0x40);

    // Limpa flags de IRQ
    spi_write_reg(handle, SX_REG_IRQ_FLAGS, 0xFF);

    // Inicia transmissão
    set_mode(handle, SX_MODE_TX);

    // Aguarda TxDone via polling (até 5s)
    // Em produção: usar interrupção no DIO0
    for (int i = 0; i < 500; i++) {
        uint8_t irq = spi_read_reg(handle, SX_REG_IRQ_FLAGS);
        if (irq & SX_IRQ_TX_DONE) {
            spi_write_reg(handle, SX_REG_IRQ_FLAGS, SX_IRQ_TX_DONE);
            set_mode(handle, SX_MODE_STDBY);
            ESP_LOGI(TAG, "TX done (%d bytes)", len);
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    ESP_LOGE(TAG, "TX timeout!");
    set_mode(handle, SX_MODE_STDBY);
    return ESP_ERR_TIMEOUT;
}

// Tenta receber um pacote (non-blocking, polling de IRQ)
// Retorna número de bytes recebidos, ou 0 se nada ainda, ou -1 em erro
int sx1276_receive(spi_device_handle_t handle, uint8_t *buf, size_t max_len, int8_t *rssi)
{
    // Se não estiver em RX contínuo, coloca agora
    uint8_t mode = spi_read_reg(handle, SX_REG_OP_MODE);
    if ((mode & 0x07) != SX_MODE_RX_CONT) {
        spi_write_reg(handle, SX_REG_DIO_MAPPING1, 0x00);  // DIO0 = RxDone
        spi_write_reg(handle, SX_REG_IRQ_FLAGS, 0xFF);
        set_mode(handle, SX_MODE_RX_CONT);
    }

    // Verifica se chegou pacote
    uint8_t irq = spi_read_reg(handle, SX_REG_IRQ_FLAGS);
    if (!(irq & SX_IRQ_RX_DONE)) {
        return 0;  // nada ainda
    }

    // Limpa flag
    spi_write_reg(handle, SX_REG_IRQ_FLAGS, SX_IRQ_RX_DONE);

    // CRC error?
    if (irq & SX_IRQ_CRC_ERROR) {
        ESP_LOGW(TAG, "CRC error no pacote recebido");
        return -1;
    }

    // Quantos bytes vieram?
    uint8_t nb = spi_read_reg(handle, SX_REG_RX_NB_BYTES);
    if (nb == 0 || nb > max_len) return -1;

    // Aponta FIFO para o início do pacote recebido
    uint8_t addr = spi_read_reg(handle, SX_REG_RX_CURR_ADDR);
    spi_write_reg(handle, SX_REG_FIFO_ADDR_PTR, addr);

    // Lê os bytes do FIFO
    for (uint8_t i = 0; i < nb; i++) {
        buf[i] = spi_read_reg(handle, SX_REG_FIFO);
    }

    // RSSI do pacote (em dBm para 433MHz: RSSI = valor - 164)
    if (rssi) {
        *rssi = (int8_t)(spi_read_reg(handle, SX_REG_PKT_RSSI) - 164);
    }

    return (int)nb;
}

// Lê o número de versão do chip (deve ser 0x12 no SX1276)
uint8_t sx1276_version(spi_device_handle_t handle)
{
    return spi_read_reg(handle, SX_REG_VERSION);
}
