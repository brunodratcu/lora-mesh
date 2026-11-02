# Circuito — ESP32 + XL1278 LoRa 433MHz

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
---
ESP32 GPIO	Fio	XL1278-SMT pad
GPIO 5	SCK	SCK
GPIO 27	MOSI	MOSI
GPIO 19	MISO	MISO
GPIO 18	CS	NSS
GPIO 14	RST	NRESET
GPIO 26	DIO0	DIO0
3.3 V	alimentação	VCC
GND	terra	GND
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
