# Circuito — ESP32 + XL1276 (SX1276) LoRa 433MHz

## Componentes

| Componente        | Qtd | Observação                              |
|-------------------|-----|-----------------------------------------|
| ESP32-WROOM-32    | 1   | Com pinos macho soldados                |
| XL1276-P01        | 1   | SX1276, 433MHz — pinos macho soldados   |
| Capacitor 100nF   | 1   | Desacoplamento na alimentação do LoRa   |
| Capacitor 10µF    | 1   | Opcional, bulk supply                   |
| Fio rígido 17,3cm | 1   | Antena quarter-wave 433MHz — OBRIGATÓRIO|
| Protoboard        | 1   | —                                       |
| Jumpers           | 8+  | —                                       |

---

## Pinagem

```
XL1276 (SX1276)    ESP32
───────────────    ─────
VCC            →   3.3V   ← NUNCA 5V, queima o módulo
GND            →   GND
SCK            →   GPIO5
MOSI           →   GPIO27
MISO           →   GPIO19
NSS (CS)       →   GPIO18
RST            →   GPIO14
DIO0           →   GPIO26
ANT            →   fio 17,3cm (antena)
```

---

## Diagrama

```
                      3.3V ──┬──── XL1276 VCC
                             │
                          [100nF]   ← perto do módulo
                             │
                       GND ──┴──── XL1276 GND

ESP32 GPIO5  ─────────────────────  XL1276 SCK
ESP32 GPIO27 ─────────────────────  XL1276 MOSI
ESP32 GPIO19 ─────────────────────  XL1276 MISO
ESP32 GPIO18 ─────────────────────  XL1276 NSS
ESP32 GPIO14 ─────────────────────  XL1276 RST
ESP32 GPIO26 ─────────────────────  XL1276 DIO0

                                    XL1276 ANT ── fio 17,3cm
```

---

## Antena

**⚠️ Nunca transmitir sem antena — danifica o PA do SX1276.**

Para 433MHz, antena quarter-wave:
- Comprimento: **λ/4 = 300 / 433 / 4 ≈ 17,3 cm**
- Material: fio de cobre rígido (AWG22-24)
- Posição: vertical, afastada de metais

---

## Notas importantes

- Alimentar o XL1276 com **3.3V** — não 5V
- Capacitor de 100nF o mais próximo possível dos pinos VCC/GND do módulo
- Verificar silkscreen do XL1276 para confirmar label de cada pino antes de soldar
- RST ativo em nível baixo: o firmware faz reset no init
