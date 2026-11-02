#pragma once

#include "sx1276.h"
#include <stdint.h>
#include <stdbool.h>

// ─── Configuração da malha ───────────────────────────────────────────────────
#define MESH_BROADCAST_ADDR   0xFF  // endereço de broadcast (todos recebem)
#define MESH_TTL_DEFAULT      3     // máximo de saltos na malha
#define MESH_MAX_PAYLOAD      56    // bytes de payload por pacote
#define MESH_MAGIC            0xAB  // identificador do protocolo

// ─── Estrutura do pacote ─────────────────────────────────────────────────────
// Total: 4 bytes de header + até 56 bytes de payload = 60 bytes máximo
typedef struct __attribute__((packed)) {
    uint8_t magic;                     // sempre MESH_MAGIC (filtra pacotes de outros)
    uint8_t src;                       // endereço de origem (1-254)
    uint8_t dst;                       // endereço de destino (1-254 ou 0xFF=broadcast)
    uint8_t ttl;                       // time-to-live: decrementado a cada salto
    uint8_t seq;                       // número de sequência (evita loop de retransmissão)
    uint8_t payload_len;               // tamanho real do payload
    uint8_t payload[MESH_MAX_PAYLOAD]; // dados
} mesh_packet_t;

// ─── API pública ─────────────────────────────────────────────────────────────
void mesh_init(spi_device_handle_t handle, uint8_t my_addr);
bool mesh_send(const char *msg, uint8_t dst);
bool mesh_send_broadcast(const char *msg);
void mesh_task(void *pvParams);  // task FreeRTOS — chame via xTaskCreate
