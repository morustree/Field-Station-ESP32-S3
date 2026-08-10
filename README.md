# ESP32-S3 Field Station

[English (US)](#english) | [Português (BR)](#português)

---
## English

### 📝 Overview
Modular firmware developed in C (**ESP-IDF**) for the **ESP32-S3 DevKitC-1 N8R2**. The system performs environmental sensor readings, processes data in external memory buffers (PSRAM), and transmits via MQTT. In case of connectivity failure or time synchronization issues, data is persisted in a **LittleFS** partition on Flash.

The station was designed for field operation, prioritizing data integrity and minimum power consumption through a "One-Shot" architecture.

---

### 📂 Project Structure

Below are the main directories and files that make up the solution:

- `main/`: Contains the main C source code.
  - `main.c`: One-Shot cycle control logic and task orchestration.
  - `*_sensor.c/h`: Modular drivers for BME280 / GY-BME280, VL53L0X / WCMCU-531 VL53L0/1XV2, and LDR 7mm.
  - `network.c/h` & `telemetry.c/h`: Wi-Fi/MQTT management and JSON data formatting.
  - `storage.c/h`: LittleFS persistence layer implementation.
- `partitions.csv`: Custom flash partition table.
- `sdkconfig.defaults`: Base hardware configuration (PSRAM, Flash, CPU).
- `frontend/`: Web Application (Dashboard) for real-time monitoring.
  - `index.html`, `app.js`, `style.css`: Interface developed in Vanilla JS with MQTT integration.
- `mosquitto/`: Configurations for the Mosquitto MQTT Broker.
- `docker-compose.yml`: Infrastructure orchestration.
- `setup.ps1`: Automated configuration script (Windows).
- `setup_config.ini`: User configuration file.

---

### 🛠️ Circuit Description & Technical Design

The electronic design focuses on I2C bus stability and the elimination of parasitic currents during deep sleep. The system is powered by an external switching power supply of +5V DC / 1A.

![Circuit Diagram](./ckt_img.png)

#### 🔌 Connection Details (Pinout)

| Component | Sensor Pin | ESP32-S3 Pin | Function / Justification |
| :--- | :--- | :--- | :--- |
| **BME280** | VCC | **3.3V** | Constant power. Enters *Sleep Mode* (0.1uA) via software. |
| | SCL / SDA | **GPIO 9 / 8** | Main I2C bus. |
| | GND | **Negative** | Common power reference. |
| **VL53L0X** | VIN | **3.3V** | Constant power. |
| | SCL / SDA | **GPIO 9 / 8** | Shared I2C bus. |
| | XSHUT | **GPIO 12** | Hardware standby control (Standby = 5uA). |
| | GND | **Negative** | Common power reference. |
| **LDR 7mm** | Terminal 1 | **GPIO 6** | **Power Gating**: The divider is only energized during reading to save power. |
| | Terminal 2 | **GPIO 4** | Analog input (**ADC1_CH3**) connected to the divider's center point. |
| **10kΩ Resistor**| Terminal 1 | **GPIO 4** | Acts as a Pull-down for the LDR. |
| | Terminal 2 | **Negative** | Common power reference. |
| **System** | +5V pin | **Power Output** | Main power from the 5V DC charger. |
| | GND pin | **Negative** | Ground interconnection with the power supply and sensors. |

#### ⚡ Power Supply and Cabling
*   **Adapter**: Model DSA-5CAA-05 (Switching Adapter).
*   **Specifications**: Input 100-240V AC | Output +5V DC 1A.
*   **Physical Connection**: A USB-A cable was modified (cut end with exposed wires) to inject power directly into the breadboard. Although the project includes a **USB-A to USB-C** cable, it is used exclusively for flashing the firmware due to its short length. The modified cable allows for the reach of the external power source and centralized power distribution.
*   **Power via UART**: The **UART** port of the ESP32-S3 DevKitC-1 accepts 5V power. However, in the final design, direct connection to the pins was chosen.

---

### 🚀 Quick Start Guide (User Manual)

#### 1. Software Prerequisites

1.  **ESP-IDF v5.x**: Official framework. **Recommended**: VS Code + Espressif IDF Extension.
2.  **Docker Desktop**: Required for the MQTT Broker.
3.  **Git**: For cloning the repository.

#### 2. Fast Setup

1.  **Clone**: Clone this repository to your computer:
    ```bash
    git clone https://github.com/morustree/Field-Station-ESP32-S3.git
    cd Field-Station-ESP32-S3
    ```
2.  **Run Setup**: Ensure **Docker Desktop** is running. Then, open PowerShell in the project root and run:
    ```powershell
    .\setup.ps1
    ```
    *The script will create your `setup_config.ini` file and open it for editing. Fill in your details, save, and run the script again to finish.*
3.  **Flash**: In the ESP-IDF terminal, run:
    ```bash
    idf.py build flash
    ```
    *Note: The `idf.py` command requires the environment variables to be set. If using **VS Code**, you can simply use the ESP-IDF extension icons (Build Project and Flash Device).*
4.  **Dashboard**: Open `frontend/index.html` in your browser. It will connect automatically.

---

### 🧠 Technologies and Resources Used

- **ESP-IDF v5.x**: Professional framework for native C development.
- **PSRAM (SPIRAM)**: Use of 2MB external RAM for JSON buffer allocation.
- **LittleFS**: Power-failure resilient file system for local telemetry storage.
- **One-Shot Architecture**: Linear life cycle (Wake up -> Measure -> Transmit -> Sleep) to maximize battery autonomy.
- **NTP/SNTP Sync**: Strict synchronization. The system validates real time before persisting or sending data to ensure chronology.
- **Silent Production**: Firmware optimized for production.

---
## Português

### 📝 Visão Geral
Firmware modular desenvolvido em C (**ESP-IDF**) para o **ESP32-S3 DevKitC-1 N8R2**. O sistema realiza a leitura de sensores ambientais, processa os dados em buffers de memória externa (PSRAM) e transmite via MQTT. Em caso de falha de conectividade ou sincronismo de tempo, os dados são persistidos em uma partição **LittleFS** na Flash.

A estação foi projetada para operação em campo, priorizando a integridade dos dados e o consumo mínimo de energia por meio de uma arquitetura "One-Shot".

---

### 📂 Estrutura do Projeto

Abaixo, os principais diretórios e arquivos que compõem a solução:

- `main/`: Contém o código-fonte principal em C.
  - `main.c`: Lógica de controle do ciclo One-Shot e orquestração de tarefas.
  - `*_sensor.c/h`: Drivers modulares para BME280 / GY-BME280, VL53L0X / WCMCU-531 VL53L0/1XV2 e LDR 7mm.
  - `network.c/h` & `telemetry.c/h`: Gestão de Wi-Fi/MQTT e formatação de dados JSON.
  - `storage.c/h`: Implementação da camada de persistência LittleFS.
- `partitions.csv`: Tabela de partições customizada.
- `sdkconfig.defaults`: Configurações base de hardware (PSRAM, Flash, CPU).
- `frontend/`: Aplicação Web (Dashboard) para monitoramento em tempo real.
  - `index.html`, `app.js`, `style.css`: Interface em Vanilla JS com integração MQTT.
- `mosquitto/`: Configurações para o Broker MQTT Mosquitto.
- `docker-compose.yml`: Orquestração da infraestrutura.
- `setup.ps1`: Script de configuração automática (Windows).
- `setup_config.ini`: Arquivo de configurações do usuário.

---

### 🛠️ Descrição do Circuito

O design eletrônico foca na estabilidade do barramento I2C e na eliminação de correntes parasitas durante o sono profundo. O sistema é alimentado por uma fonte externa chaveada de +5V DC / 1A.

![Circuit Diagram](./ckt_img.png)

#### 🔌 Detalhamento de Conexões (Pinagem)

| Componente            | Pino Sensor | Pino ESP32-S3 | Função / Justificativa |
|:----------------------| :--- | :--- | :--- |
| **BME280**            | VCC | **3.3V** | Alimentação constante. Entra em *Sleep Mode* (0.1uA) via software. |
|                       | SCL / SDA | **GPIO 9 / 8** | Barramento I2C principal. |
|                       | GND | **Negativo** | Referência comum de energia. |
| **VL53L0X**           | VIN | **3.3V** | Alimentação constante. |
|                       | SCL / SDA | **GPIO 9 / 8** | Barramento I2C compartilhado. |
|                       | XSHUT | **GPIO 12** | Controle de standby via hardware (Standby = 5uA). |
|                       | GND | **Negativo** | Referência comum de energia. |
| **LDR 7mm**           | Terminal 1 | **GPIO 6** | **Power Gating**: O divisor só é energizado durante a leitura para economizar energia. |
|                       | Terminal 2 | **GPIO 4** | Entrada analógica (**ADC1_CH3**) conectada ao ponto central do divisor. |
| **Resistor 10kΩ**     | Terminal 1 | **GPIO 4** | Atua como Pull-down para o LDR. |
|                       | Terminal 2 | **Negativo** | Referência comum de energia. |
| **Sistema**           | pino +5V | **Saída Fonte** | Alimentação principal vinda do carregador 5V DC. |
|                       | pino GND | **Negativo** | Interconexão de massa com a fonte e sensores. |

#### ⚡ Fonte de Alimentação e Cabeamento
*   **Adaptador**: Modelo DSA-5CAA-05 (Switching Adapter).
*   **Especificações**: Input 100-240V AC | Output +5V DC 1A.
*   **Conexão Física**: Um cabo USB-A foi modificado (extremidade cortada com fios expostos) para injetar a alimentação diretamente na protoboard. Embora o projeto conte com um cabo **USB-A para USB-C**, este é utilizado exclusivamente para gravação do firmware devido ao seu comprimento reduzido. O cabo modificado permite o alcance da fonte externa e a distribuição centralizada de energia.
*   **Alimentação via UART**: A porta **UART** do ESP32-S3 DevKitC-1 aceita alimentação de 5V. No entanto, no design final, optou-se pela conexão direta nos pinos.

---

### 🚀 Guia de Início Rápido (Manual de Uso)

#### 1. Pré-requisitos de Software

1.  **ESP-IDF v5.x**: Framework oficial. **Recomendado**: VS Code + Extensão Espressif IDF.
2.  **Docker Desktop**: Necessário para o Broker MQTT local.
3.  **Git**: Para clonagem do repositório.

#### 2. Configuração Rápida

1.  **Clonar**: Clone este repositório em seu computador:
    ```bash
    git clone https://github.com/morustree/Field-Station-ESP32-S3.git
    cd Field-Station-ESP32-S3
    ```
2.  **Executar Setup**: Certifique-se de que o **Docker Desktop** esteja rodando. Então, abra o PowerShell na raiz do projeto e execute:
    ```powershell
    .\setup.ps1
    ```
    *O script criará o arquivo `setup_config.ini` e o abrirá para edição. Preencha seus dados, salve e execute o script novamente para concluir.*
3.  **Gravar**: No terminal do ESP-IDF, execute:
    ```bash
    idf.py build flash
    ```
    *Nota: O comando `idf.py` exige que as variáveis de ambiente estejam configuradas. Se estiver usando o **VS Code**, você pode simplesmente usar os ícones da extensão ESP-IDF (Build e Flash).*
4.  **Dashboard**: Abra `frontend/index.html` no navegador. Ele conectará automaticamente.

---

### 🧠 Tecnologias e Recursos Utilizados

- **ESP-IDF v5.x**: Framework profissional para desenvolvimento nativo em C.
- **PSRAM (SPIRAM)**: Uso de 2MB de RAM externa para alocação de buffers JSON.
- **LittleFS**: Sistema de arquivos resiliente a falhas de energia para armazenamento local de telemetria.
- **One-Shot Architecture**: Ciclo de vida linear (Acordar -> Medir -> Transmitir -> Dormir) para maximizar a autonomia da bateria.
- **NTP/SNTP Sync**: Sincronização estrita. O sistema valida o tempo real antes de persistir ou enviar dados para garantir a cronologia.
- **Produção Silenciosa**: Firmware otimizado para produção.

---
