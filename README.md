# Projeto-C115-Sistema-de-Irrigação

Este repositório contém o código-fonte para um sistema de irrigação automatizado utilizando sensores de umidade, LDR, eletrodos e controle via Wi-Fi e MQTT. O projeto é dividido em dois arquivos principais, cada um com configurações específicas.

Desenvolvido por: Lara Conte Gomes e Lívia Cecília Gomes Silva

## Estrutura do Repositório

- `codigo_estufa/`
  - **codigo_estufa.ino**: Código para controle de sensores e motor utilizando Wi-Fi e MQTT.
- `Codigo_Estufa_Projeto_C115/`
  - **Codigo_Estufa_Projeto_C115.ino**: Código alternativo com configurações diferentes de Wi-Fi e servidor MQTT.

## Funcionalidades

- **Sensores**:
  - Sensores de umidade para monitoramento do solo.
  - LDR para detecção de luminosidade.
  - Eletrodo para detecção de nível de água.

- **Atuadores**:
  - Controle de motor para irrigação.
  - LED indicador.

- **Conectividade**:
  - Conexão Wi-Fi para comunicação com servidor MQTT.
  - Publicação e assinatura de tópicos MQTT para controle remoto.

## Configurações

### Arquivo: `codigo_estufa.ino`

- **Wi-Fi**:
  - SSID: `MechBerry`
  - Senha: `H4x7Z:NF`

- **MQTT**:
  - Servidor: `192.168.40.6`
  - Porta: `1883`
  - Tópico: `MechBerry/#`

- **Calibração de Sensores**:
  - Valores mínimos (em água): `{1749, 1469, 1419, 1419}`
  - Valores máximos (ao ar livre): `{3553, 3575, 3507, 3438}`

### Arquivo: `Codigo_Estufa_Projeto_C115.ino`

- **Wi-Fi**:
  - SSID: `WLL-Inatel`
  - Senha: `inatelsemfio`

- **MQTT**:
  - Servidor: `test.mosquitto.org`
  - Porta: `1883`
  - Tópico: `MechBerry/#`

- **Calibração de Sensores**:
  - Valores mínimos (em água): `{1749, 1469, 1419, 1419}`

## Como Usar

1. **Configuração do Ambiente**:
   - Certifique-se de ter o Arduino IDE instalado.
   - Instale as bibliotecas necessárias:
     - `WiFi.h`
     - `PubSubClient.h`

2. **Carregamento do Código**:
   - Escolha o arquivo `.ino` apropriado para sua configuração.
   - Conecte o dispositivo ao computador e carregue o código.

3. **Operação**:
   - Conecte os sensores e atuadores aos pinos especificados.
   - Ligue o dispositivo e monitore a comunicação via MQTT.

## Contribuição

Contribuições são bem-vindas! Sinta-se à vontade para abrir issues ou enviar pull requests.

## Licença

Este projeto está licenciado sob a licença MIT. Consulte o arquivo `LICENSE` para mais detalhes.
