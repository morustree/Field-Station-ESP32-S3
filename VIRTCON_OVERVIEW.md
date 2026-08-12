# Virtcon: Plataforma de Monitoramento e Detecção de Anomalias

Este documento consolida as informações técnicas e funcionais do sistema **Virtcon**.

---

## 1. Visão Geral (SaaS Multi-tenant)

O **Virtcon** é uma plataforma SaaS (Software as a Service) multi-tenant projetada para monitoramento e detecção de anomalias em diversos cenários. O sistema combina ingestão agnóstica de dados IoT, modelos estatísticos avançados e inteligência artificial generativa para transformar dados brutos de sensores em diagnósticos. A plataforma é **agnóstica ao tipo de métrica**, permitindo o monitoramento de qualquer conjunto de até 4 variáveis simultâneas.

### Arquitetura e Stack Tecnológica
*   **Backend:** Kotlin 2.3.21, Spring Boot 4.1.0 e Java 25. Processamento assíncrono para análise de dados e integração com LLM via WebFlux.
*   **Frontend:** React 19.2 com TanStack Start (SSR via Nitro), TypeScript 5.8 e Tailwind CSS 4.
*   **Infraestrutura:** PostgreSQL 16 (metadados), InfluxDB 2.7 (séries temporais), Redis 7 (cache e histórico de chat) e Mosquitto 2.0 (Broker MQTT).
*   **Segurança:** Isolamento multi-tenant via Hibernate Filters e AOP. Autenticação JWT via cookies HttpOnly Secure.

---

## 2. Metodologia Técnica: Isolation Forest e IA

O núcleo analítico do Virtcon utiliza a família de algoritmos **Isolation Forest (iForest)**, implementada via biblioteca Smile (*Statistical Machine Intelligence and Learning Engine*), para identificação de desvios.

### Modelos de Análise
1.  **iForest Padrão:** Isola anomalias usando partições aleatórias alinhadas aos eixos.
2.  **Extended Isolation Forest (EIF):** Utiliza hiperplanos com inclinações aleatórias para mitigar artefatos em dados correlacionados.
3.  **Rotated Isolation Forest (RIF):** Aplica transformações lineares aleatórias (rotações via decomposição QR) antes do isolamento, sendo robusto para anomalias complexas não alinhadas aos eixos.
4.  **Em Desenvolvimento (Roadmap):** Inclusão de modelos **DBSCAN** e **Z-Score**.
*   **Salvaguarda:** Downsampling adaptativo para até 5000 pontos com notificação de agregação aplicada (`X-Aggregation-Applied`).

### Diagnóstico Assistido por IA (Google Gemini)
O sistema integra o Google Gemini para traduzir scores matemáticos em diagnósticos em linguagem natural. Utiliza uma abordagem BYOK (Bring Your Own Key) com criptografia AES-256-GCM para as chaves dos inquilinos, garantindo privacidade e soberania de dados.

---

## 3. Funcionalidades Principais

### Ingestão Agnóstica de Telemetria
Suporta o envio de dados em tempo real sem travas de frequência:
*   **HTTPS (REST API):** Comunicação segura sobre TLS 1.3 utilizando o Device ID (UUID) como credencial única, simplificando a integração de hardware.
*   **MQTT:** Ideal para dispositivos IoT de baixo consumo (ESP32/Raspberry Pi) com autenticação baseada em identificadores de dispositivo.

---

## 4. Estação IoT: Field-Station-ESP32-S3

O ecossistema Virtcon inclui uma implementação, a **[Field-Station-ESP32-S3](https://github.com/morustree/Field-Station-ESP32-S3)**. Esta estação meteorológica IoT foi projetada para demonstrar a viabilidade da detecção de anomalias em condições reais.

### Características Técnicas do Hardware
*   **Microcontrolador:** ESP32-S3 (DevKitC-1 N8R2) com suporte a SPIRAM para gestão eficiente de payloads JSON.
*   **Sensoriamento:** Integração via I2C com o sensor **BME280** (Temperatura, Pressão e Umidade) e um sensor **LDR** para medição de luminosidade via ADC.
*   **Eficiência Energética:** Arquitetura One-Shot baseada em Deep Sleep profundo, otimizando o consumo de bateria para operações em campo.

### Resiliência e Integração Analítica
*   **Persistência Local:** Utiliza o sistema de arquivos LittleFS para armazenar telemetria em Flash em caso de falhas temporárias de conectividade Wi-Fi ou sincronização NTP.
*   **Detecção de Mudanças Abruptas:** A estação fornece o fluxo de dados necessário para que os motores do Virtcon identifiquem mudanças ambientais súbitas.

---

## 5. Importação No-Code de Dados Históricos (planilhas)
Pipeline robusto de duas fases (ANALYZE → COMMIT) para carga de grandes volumes (até 1M linhas):
*   **Mapeamento Inteligente:** Algoritmo Jaro-Winkler para identificação automática de cabeçalhos.
*   **Integridade Atômica:** Transações ACID que sincronizam bancos relacionais e de séries temporais.
*   **Resolução de Conflitos:** Interface visual para gerenciar divergências de localização e tags de sensores.

---

## 6. Status do Projeto
O Virtcon encontra-se em estágio inicial de desenvolvimento (**Proof of Concept / MVP**). Embora o núcleo analítico e de ingestão esteja funcional, os seguintes recursos estão em fase de implementação ou refinamento:
*   **Gemini AI:** Integração em processamento (o motor de diagnóstico ainda não está ativo em produção).
*   **Exportação:** Funcionalidade de exportação de dados em CSV em desenvolvimento.
*   **Visualização:** Aprimoramento das visualizações do dashboard (iForest e telemetria).

---

## 7. Interface e Visualização

Gráficos que permitem a correlação visual entre as anomalias e as variáveis.

![iForest](./images/iforest.png)

---

![Infrastructure Map](./images/infrastructure-map.png)

---

