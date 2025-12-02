#include <WiFi.h>
#include <PubSubClient.h>

// Definição de pinos
const int pinoLed = 26;       // Pino digital para o LED
const int ldrPin = 32;        // Pino analógico para o LDR
const int pinoEletrodo = 35;  // Pino analógico para o eletrodo

#define SENSOR_1 33
#define SENSOR_2 34
#define SENSOR_3 36
#define SENSOR_4 39

#define MOTOR_PIN 25          // Pino para acionar o motor

// Configurações Wi-Fi
const char* ssid = "WLL-Inatel";
const char* password = "inatelsemfio";

// Configurações MQTT (Mosquitto padrão)
const char* mqtt_server = "test.mosquitto.org";  // Servidor Mosquitto
const int mqttPort = 1883;                 // Porta padrão
const char* mqttUser = "";                 // Sem autenticação
const char* mqttPassword = "";
const char* mqttTopicSub = "MechBerry/#";   // Tópico para se inscrever

WiFiClient espClient;
PubSubClient client(espClient);

// Variáveis de sensores
int valorLDR = 0;
int limiteLDR = 800;

int valorEletrodo = 0;
int limiteAgua = 500;

const int umidadeLimite = 50;

// Valores calibrados (em água e ar)
int min_agua[] = {1749, 1469, 1419, 1419};
int max_ar[]   = {3553, 3575, 3507, 3438};

// Função para calcular a umidade
int calcularUmidade(int valor_lido, int min, int max) {
  int umidade = map(valor_lido, min, max, 100, 0);
  if (umidade > 100) umidade = 100;
  if (umidade < 0) umidade = 0;
  return umidade;
}

// Conexão WiFi
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
    Serial.println("\nFalha ao conectar no WiFi");
  }
}

// CALLBACK MQTT ────────────────
void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) msg += (char)payload[i];

  Serial.print("\n📩 Mensagem recebida em: ");
  Serial.println(topic);
  Serial.print("Payload: ");
  Serial.println(msg);

  // Leitura dos sensores
  valorEletrodo = analogRead(pinoEletrodo);

  int umidade1 = calcularUmidade(analogRead(SENSOR_1), min_agua[0], max_ar[0]);
  int umidade2 = calcularUmidade(analogRead(SENSOR_2), min_agua[1], max_ar[1]);
  int umidade3 = calcularUmidade(analogRead(SENSOR_3), min_agua[2], max_ar[2]);
  int umidade4 = calcularUmidade(analogRead(SENSOR_4), min_agua[3], max_ar[3]);

  // ─────────────────────────────────────────────
  // ROTAS MQTT
  // ─────────────────────────────────────────────
  if (strcmp(topic, "MechBerry/Irrigacao/Acionar") == 0) {
    digitalWrite(MOTOR_PIN, LOW);
    Serial.println("Motor acionado!");

    client.publish("MechBerry/Irrigacao/Resposta",
                   "{\"status\":1,\"mensagem\":\"Irrigação acionada\"}");

  } else if (strcmp(topic, "MechBerry/Irrigacao/Desacionar") == 0) {
    digitalWrite(MOTOR_PIN, HIGH);
    Serial.println("Motor desacionado!");

    client.publish("MechBerry/Irrigacao/Resposta",
                   "{\"status\":1,\"mensagem\":\"Irrigação desacionada\"}");

  } else if (strcmp(topic, "MechBerry/Irrigacao/Infos") == 0) {

    // Umidade baixa?
    if (umidade1 < umidadeLimite || umidade2 < umidadeLimite ||
        umidade3 < umidadeLimite || umidade4 < umidadeLimite) {
      
      client.publish("MechBerry/Irrigacao/Informado",
                     "{\"status\":1,\"umidade\":\"baixa\",\"acao\":\"Irrigação necessária\"}");
      return;
    }

    // Água insuficiente?
    if (valorEletrodo < limiteAgua) {
      client.publish("MechBerry/Irrigacao/Informado",
                     "{\"status\":0,\"nivel_agua\":\"baixo\",\"erro\":\"Nível de água insuficiente\"}");
      return;
    }

    // Tudo OK
    client.publish("MechBerry/Irrigacao/Informado",
                   "{\"status\":1,\"umidade\":\"adequada\",\"nivel_agua\":\"adequado\",\"acao\":\"Condições ideais\"}");
  }
}

// Função de conexão ao MQTT
void connect() {
  while (!client.connected()) {
    Serial.print("Conectando ao Broker MQTT...");

    if (client.connect("ESP32Client", mqttUser, mqttPassword)) {
      Serial.println("Conectado!");

      // Assinatura dos tópicos principais
      client.subscribe("MechBerry/Irrigacao/Acionar");
      client.subscribe("MechBerry/Irrigacao/Desacionar");
      client.subscribe("MechBerry/Irrigacao/Infos");

    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 2 segundos...");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(pinoLed, OUTPUT);
  digitalWrite(pinoLed, LOW);

  pinMode(ldrPin, INPUT);
  pinMode(pinoEletrodo, INPUT);

  pinMode(SENSOR_1, INPUT);
  pinMode(SENSOR_2, INPUT);
  pinMode(SENSOR_3, INPUT);
  pinMode(SENSOR_4, INPUT);

  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, HIGH);

  setup_wifi();

  client.setServer(mqtt_server, mqttPort);
  client.setCallback(callback);

  connect();
}

void loop() {
  if (!client.connected()) {
    connect();
  }
  client.loop();

  // Leitura do LDR
  valorLDR = analogRead(ldrPin);
  Serial.print("Luminosidade: ");
  Serial.println(valorLDR);

  int dutycycle = map(valorLDR, 200, 700, 0, 255);
  dutycycle = constrain(dutycycle, 0, 255);

  ledcWrite(0, dutycycle);

  delay(1000);
}
