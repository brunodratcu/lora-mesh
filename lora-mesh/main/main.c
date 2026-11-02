#include "mesh.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MAIN";

// ─── CONFIGURAÇÃO ─────────────────────────────────────────────────────────────
// Defina um endereço único para cada nó da malha (1-254)
// Nó A: #define MY_ADDR 0x01
// Nó B: #define MY_ADDR 0x02
#define MY_ADDR  0x01

// Defina o endereço do destino padrão (ou MESH_BROADCAST_ADDR para todos)
#define DST_ADDR MESH_BROADCAST_ADDR
// ─────────────────────────────────────────────────────────────────────────────

// Task que envia mensagens periodicamente (para testes)
static void tx_task(void *pvParams)
{
    uint32_t count = 0;
    char msg[48];

    // Aguarda um momento para o rádio estabilizar
    vTaskDelay(pdMS_TO_TICKS(2000));

    while (1) {
        snprintf(msg, sizeof(msg), "ola da mesh! no=0x%02X count=%lu", MY_ADDR, count++);
        mesh_send(msg, DST_ADDR);
        vTaskDelay(pdMS_TO_TICKS(5000));  // transmite a cada 5s
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== LoRa Mesh ESP-IDF ===");
    ESP_LOGI(TAG, "Endereco deste no: 0x%02X", MY_ADDR);

    // 1. Inicializa SPI e obtém handle do device
    spi_device_handle_t spi;
    ESP_ERROR_CHECK(spi_driver_init(&spi));

    // 2. Inicializa o rádio SX1276
    esp_err_t ret = sx1276_init(spi);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao inicializar SX1276. Verifique a pinagem e a antena.");
        return;
    }

    // 3. Inicializa a camada de malha com o endereço deste nó
    mesh_init(spi, MY_ADDR);

    // 4. Cria a task de recepção/roteamento (prioridade alta)
    xTaskCreate(mesh_task, "mesh_rx", 4096, NULL, 5, NULL);

    // 5. Cria a task de transmissão periódica (para testes)
    xTaskCreate(tx_task, "mesh_tx", 2048, NULL, 3, NULL);

    ESP_LOGI(TAG, "Tasks iniciadas. Monitorando...");
}
