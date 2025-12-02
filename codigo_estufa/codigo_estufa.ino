#include <WiFi.h>
#include <PubSubClient.h>

// Definição de pinos
const int pinoLed = 26;       // Pino digital para o LED
const int ldrPin = 32;        // Pino analógico para o LDR
const int pinoEletrodo = 35;  // Pino analógico para o eletrodo
#define SENSOR_1 33           // Sensor de umidade 1
#define SENSOR_2 34           // Sensor de umidade 2
#define SENSOR_3 36           // Sensor de umidade 3
#define SENSOR_4 39           // Sensor de umidade 4
#define MOTOR_PIN 25          // Pino digital para acionar o motor

// Configurações Wi-Fi e MQTT
const char* ssid = "MechBerry";
const char* password = "H4x7Z:NF";
const char* mqtt_server = "192.168.40.6";  // Servidor MQTT
const int mqttPort = 1883;                  // Porta MQTT
const char* mqttTopicSub = "MechBerry/#";   // Tópico para se inscrever

WiFiClient espClient;
PubSubClient client(espClient);

int valorLDR = 0;
int limiteLDR = 800;
int valorEletrodo = 0;
int limiteAgua = 500;
const int umidadeLimite = 50;  // 50% de umidade como limite

// Definindo os valores mínimos (na água) e máximos (ao ar livre) para calibração
int min_agua[] = {1749, 1469, 1419, 1419};  // Valores mínimos (em água)
int max_ar[] = {3553, 3575, 3507, 3438};    // Valores máximos (ao ar livre)

// Função para calcular a umidade calibrada em porcentagem
int calcularUmidade(int valor_lido, int min, int max) {
  int umidade = map(valor_lido, min, max, 100, 0); // Mapear o valor lido entre 0% e 100%
  if (umidade > 100) umidade = 100;  // Limitar o valor máximo a 100%
  if (umidade < 0) umidade = 0;      // Limitar o valor mínimo a 0%
  return umidade;
}

// Função para conectar ao Wi-Fi com timeout
void setup_wifi() {
  Serial.print("Conectando-se a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi conectado");
    Serial.print("Endereço IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFalha na conexão WiFi");
  }
}

// Função de callback para processar mensagens recebidas
void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.print("Mensagem recebida em: ");
  Serial.println(topic);
  Serial.print("Mensagem: ");
  Serial.println(msg);

  valorEletrodo = analogRead(pinoEletrodo);
  Serial.print("Nível de água: ");
  Serial.println(valorEletrodo);

  // Leitura e cálculo da umidade calibrada para cada sensor
  int umidade1 = calcularUmidade(analogRead(SENSOR_1), min_agua[0], max_ar[0]);
  int umidade2 = calcularUmidade(analogRead(SENSOR_2), min_agua[1], max_ar[1]);
  int umidade3 = calcularUmidade(analogRead(SENSOR_3), min_agua[2], max_ar[2]);
  int umidade4 = calcularUmidade(analogRead(SENSOR_4), min_agua[3], max_ar[3]);

  Serial.print("Umidade Sensor 1: ");
  Serial.println(umidade1);
  Serial.print("Umidade Sensor 2: ");
  Serial.println(umidade2);
  Serial.print("Umidade Sensor 3: ");
  Serial.println(umidade3);
  Serial.print("Umidade Sensor 4: ");
  Serial.println(umidade4);

  // Controle de acionamento da irrigação
  if (strcmp(topic, "MechBerry/Irrigacao/Acionar") == 0) {
    digitalWrite(MOTOR_PIN, LOW);  // Liga o motor
    Serial.println("Irrigação acionada via aplicativo.");
    client.publish("MechBerry/Irrigacao/Resposta", "{\"status\": 1}");
  } else if (strcmp(topic, "MechBerry/Irrigacao/Desacionar") == 0) {
    digitalWrite(MOTOR_PIN, HIGH);  // Desliga o motor
    Serial.println("Irrigação desacionada via aplicativo.");
    client.publish("MechBerry/Irrigacao/Resposta", "{\"status\": 1}");
  } else if (strcmp(topic, "MechBerry/Irrigacao/Infos") == 0) {
    // Verifica se algum sensor está com umidade abaixo do limite
    if (umidade1 < umidadeLimite || umidade2 < umidadeLimite || umidade3 < umidadeLimite || umidade4 < umidadeLimite) {
      client.publish("MechBerry/Irrigacao/Informado", "{\"status\": 1, \"umidade\": \"baixa\", \"acao\": \"Irrigação necessária\"}");
    } else if (valorEletrodo < limiteAgua) {
      client.publish("MechBerry/Irrigacao/Informado", "{\"status\": 0, \"nivel_agua\": \"baixo\", \"erro\": \"Nível de água insuficiente\"}");
    } else {
      client.publish("MechBerry/Irrigacao/Informado", "{\"status\": 1, \"umidade\": \"adequada\", \"nivel_agua\": \"adequado\", \"acao\": \"Condições ideais\"}");
    }
  }
}

// Função de conexão ao MQTT
void connect() {
  while (!client.connected()) {
    Serial.print("Conectando ao Broker MQTT...");

    if (client.connect("ESP32Client")) {
      Serial.println("Conectado ao MQTT!");
      client.subscribe(mqttTopicSub);
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 2 segundos");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Configura os pinos
  pinMode(pinoLed, OUTPUT);  // Use OUTPUT em vez de ledcSetup para testar
  digitalWrite(pinoLed, LOW);  // Inicialmente, o LED está desligado
  pinMode(ldrPin, INPUT);
  pinMode(pinoEletrodo, INPUT);
  pinMode(SENSOR_1, INPUT);
  pinMode(SENSOR_2, INPUT);
  pinMode(SENSOR_3, INPUT);
  pinMode(SENSOR_4, INPUT);
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, HIGH);  // Motor desligado inicialmente

  setup_wifi();  // Conectando ao Wi-Fi

  client.setServer(mqtt_server, mqttPort);  // Configura o servidor MQTT
  client.setCallback(callback);             // Define a função de callback

  connect();  // Conectar ao broker MQTT
}


void loop() {

  if (!client.connected()) {
    connect();  // Reconectar ao broker MQTT se necessário
  }
  client.loop();

  valorLDR = analogRead(ldrPin);
  Serial.print("Luminosidade: ");
  Serial.println(valorLDR);

  int dutycycle = map(valorLDR, 200, 700, 0, 255);
  if (dutycycle < 0){
    dutycycle = 0; 
  }
  else if (dutycycle > 255)
    dutycycle = 255; 
  ledcWrite(0, dutycycle);

  // Aguardando um segundo antes da próxima leitura
  delay(1000);
}