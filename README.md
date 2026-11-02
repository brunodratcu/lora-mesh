# LoRa Mesh — ESP32 + SX1276 (XL1276)

Implementação from scratch de uma malha LoRa 433MHz usando ESP-IDF.  
Sem bibliotecas externas — driver SPI e protocolo escritos à mão para fins de aprendizado.

---

## Hardware

- ESP32-WROOM-32
- XL1278-SMT (XL1278 433MHz)
- Capacitor 100nF (desacoplamento)
- Fio 17,3cm como antena quarter-wave

Ver `circuit.md` para o diagrama completo de conexões.

---

## Estrutura do projeto

```
lora-mesh/
├── main/
│   ├── main.c          → entry point, tasks FreeRTOS
│   ├── spi_driver.c/h  → driver SPI raw para o SX1276
│   ├── sx1276.c/h      → driver do rádio (registradores, TX, RX)
│   └── mesh.c/h        → protocolo de malha (pacote, roteamento, TTL)
├── CMakeLists.txt
├── circuit.md
└── README.md
```

---

## Instalação do ambiente

### 1. VSCode + extensão ESP-IDF

1. Instala o [VSCode](https://code.visualstudio.com/)
2. Instala a extensão **ESP-IDF** (Espressif Systems)
3. Abre o Command Palette (`Ctrl+Shift+P`) → `ESP-IDF: Configure ESP-IDF Extension`
4. Escolhe `Express` → versão `v5.x` → aguarda o download do toolchain (~5 min)

### 2. Clona o projeto

```bash
git clone https://github.com/SEU_USUARIO/lora-mesh.git
cd lora-mesh
```

### 3. Abre no VSCode

```
File → Open Folder → seleciona a pasta lora-mesh
```

---

## Configuração

Antes de compilar, edita `main/main.c`:

```c
// Endereço único deste nó na malha (1-254)
// Nó A usa 0x01, Nó B usa 0x02, etc.
#define MY_ADDR  0x01

// Destino padrão para mensagens de teste
// MESH_BROADCAST_ADDR (0xFF) = todos os nós
#define DST_ADDR MESH_BROADCAST_ADDR
```

---

## Compilar e gravar

No VSCode com a extensão ESP-IDF:

1. Seleciona a porta serial do ESP32 (barra inferior)
2. `Ctrl+Shift+P` → `ESP-IDF: Build, Flash and Monitor`

Ou via terminal:

```bash
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor   # Linux/Mac
idf.py -p COM3 flash monitor            # Windows
```

---

## O que esperar no monitor serial

**Primeiro boot (XL1278 detectado):**
```
I (320) SPI: SPI inicializado — SCK:5 MOSI:27 MISO:19 CS:18 RST:14
I (340) XL1278: XL1278 detectado. Version=0x12
I (360) XL1278: XL1278 configurado: 433MHz SF7 BW125 CR4/5 CRC:on
I (380) MESH: Mesh iniciada — endereco: 0x01
I (400) MESH: Task de recepcao iniciada
I (2400) MESH: Enviado para 0xFF: "ola da mesh! no=0x01 count=0"
```

**Se receber mensagem do outro nó:**
```
I (xxxx) MESH: MENSAGEM de 0x02 [RSSI:-65dBm]: "ola da mesh! no=0x02 count=0"
```

**Se XL1278 não responder:**
```
E (340) XL1278: SX1276 nao encontrado! Version=0x00 (esperado 0x12)
E (340) XL1278: Verifique a pinagem e a solda.
```
→ Revisar conexões conforme `circuit.md`

---

## Como funciona a malha

### Protocolo de pacote

```
Byte 0: magic (0xAB) — identifica o protocolo
Byte 1: src           — endereço do nó de origem
Byte 2: dst           — endereço do destino (0xFF = broadcast)
Byte 3: ttl           — time-to-live (decrementado a cada salto)
Byte 4: seq           — número de sequência (anti-loop)
Byte 5: payload_len   — tamanho da mensagem
Byte 6+: payload      — mensagem (até 56 bytes)
```

### Roteamento

1. Nó A transmite pacote com `ttl=3`
2. Nó B recebe, está no caminho: decrementa TTL, retransmite
3. Nó C é o destino: entrega a mensagem
4. Cache de `(src, seq)` evita que nós retransmitam o mesmo pacote em loop

```
A (tx) ──LoRa──▶ B (relay, ttl-1) ──LoRa──▶ C (destino)
```

---

## Segundo nó

Para ter comunicação real, o amigo monta o mesmo circuito e grava o mesmo firmware com `MY_ADDR 0x02`.  
Não há configuração de rede — os nós se descobrem automaticamente pelo rádio.

---

## Licença

MIT — livre para usar, modificar e distribuir.
