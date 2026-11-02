#include "mesh.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "MESH";

// Estado interno da malha
static spi_device_handle_t s_spi;
static uint8_t             s_my_addr;
static uint8_t             s_seq = 0;

// Cache de pacotes vistos recentemente (evita loop de retransmissão)
// Um nó não retransmite pacote que já viu (mesmo src+seq)
#define SEEN_CACHE_SIZE 16
typedef struct { uint8_t src; uint8_t seq; } seen_entry_t;
static seen_entry_t s_seen[SEEN_CACHE_SIZE];
static uint8_t      s_seen_idx = 0;

static bool already_seen(uint8_t src, uint8_t seq)
{
    for (int i = 0; i < SEEN_CACHE_SIZE; i++) {
        if (s_seen[i].src == src && s_seen[i].seq == seq) return true;
    }
    return false;
}

static void mark_seen(uint8_t src, uint8_t seq)
{
    s_seen[s_seen_idx % SEEN_CACHE_SIZE] = (seen_entry_t){ src, seq };
    s_seen_idx++;
}

// Inicializa a camada de malha
void mesh_init(spi_device_handle_t handle, uint8_t my_addr)
{
    s_spi     = handle;
    s_my_addr = my_addr;
    memset(s_seen, 0, sizeof(s_seen));
    ESP_LOGI(TAG, "Mesh iniciada — endereco: 0x%02X", my_addr);
}

// Monta e envia um pacote
bool mesh_send(const char *msg, uint8_t dst)
{
    mesh_packet_t pkt = {
        .magic       = MESH_MAGIC,
        .src         = s_my_addr,
        .dst         = dst,
        .ttl         = MESH_TTL_DEFAULT,
        .seq         = s_seq++,
        .payload_len = 0,
    };

    size_t msg_len = strlen(msg);
    if (msg_len > MESH_MAX_PAYLOAD) msg_len = MESH_MAX_PAYLOAD;
    memcpy(pkt.payload, msg, msg_len);
    pkt.payload_len = (uint8_t)msg_len;

    size_t pkt_size = sizeof(mesh_packet_t) - MESH_MAX_PAYLOAD + msg_len;
    mark_seen(s_my_addr, pkt.seq);

    esp_err_t ret = sx1276_send(s_spi, (uint8_t *)&pkt, pkt_size);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Enviado para 0x%02X: \"%.*s\"", dst, (int)msg_len, msg);
        return true;
    }
    return false;
}

// Atalho para broadcast
bool mesh_send_broadcast(const char *msg)
{
    return mesh_send(msg, MESH_BROADCAST_ADDR);
}

// Task FreeRTOS: recebe pacotes e decide o que fazer com eles
// Rodar com: xTaskCreate(mesh_task, "mesh", 4096, NULL, 5, NULL);
void mesh_task(void *pvParams)
{
    uint8_t  buf[sizeof(mesh_packet_t)];
    int8_t   rssi;

    ESP_LOGI(TAG, "Task de recepcao iniciada");

    while (1) {
        int nb = sx1276_receive(s_spi, buf, sizeof(buf), &rssi);

        if (nb <= 0) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        if (nb < 6) {
            // Pacote pequeno demais para ter header completo
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        mesh_packet_t *pkt = (mesh_packet_t *)buf;

        // Filtra pacotes que não são do nosso protocolo
        if (pkt->magic != MESH_MAGIC) continue;

        // Filtra pacotes que este nó já viu (anti-loop)
        if (already_seen(pkt->src, pkt->seq)) continue;
        mark_seen(pkt->src, pkt->seq);

        bool for_me = (pkt->dst == s_my_addr || pkt->dst == MESH_BROADCAST_ADDR);

        if (for_me) {
            // Pacote é para este nó — processa
            char msg[MESH_MAX_PAYLOAD + 1];
            memcpy(msg, pkt->payload, pkt->payload_len);
            msg[pkt->payload_len] = '\0';

            ESP_LOGI(TAG, "MENSAGEM de 0x%02X [RSSI:%ddBm]: \"%s\"",
                     pkt->src, rssi, msg);
        }

        // Retransmite se TTL ainda permite e não é destino final exclusivo
        if (pkt->ttl > 1 && pkt->dst != s_my_addr) {
            pkt->ttl--;
            size_t pkt_size = sizeof(mesh_packet_t) - MESH_MAX_PAYLOAD + pkt->payload_len;
            esp_err_t ret = sx1276_send(s_spi, buf, pkt_size);
            if (ret == ESP_OK) {
                ESP_LOGD(TAG, "Retransmitido src=0x%02X dst=0x%02X ttl=%d",
                         pkt->src, pkt->dst, pkt->ttl);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
