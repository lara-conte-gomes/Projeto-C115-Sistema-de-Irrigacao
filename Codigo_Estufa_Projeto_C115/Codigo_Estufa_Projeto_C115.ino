#include <WiFi.h>
#include <PubSubClient.h>

// === PINAGEM ===
#define MOTOR_PIN 25
const int pinoLed = 26;
const int ldrPin = 32;
const int pinoEletrodo = 35;

#define SENSOR_2 34
#define SENSOR_4 39

// === CONFIG WI-FI ===
const char* ssid = "CSI-Lab";
const char* password = "In@teLCS&I";

// === CONFIG MQTT ===
const char* mqtt_server = "test.mosquitto.org";
const int mqttPort = 1883;
const char* mqttTopicSub = "MechBerry/Irrigacao/#";

WiFiClient espClient;
PubSubClient client(espClient);

// === VARIÁVEIS ===
int limiteLDR = 800;
int limiteAgua = 500;

// Calibração SENSOR_2 e SENSOR_4
int min_agua[] = {1469, 1419}; 
int max_ar[]   = {3575, 3438}; 

// Controle de intervalo de leitura
unsigned long ultimoUpdate = 0;
const unsigned long intervaloLeitura = 1000; // 1 segundo

// ================== FUNÇÃO DE UMIDADE ==================
int calcularUmidade(int valor_lido, int min_val, int max_val) {
  int umid = map(valor_lido, min_val, max_val, 100, 0);
  if (umid > 100) umid = 100;
  if (umid < 0) umid = 0;
  return umid;
}

// ------------------ WIFI ------------------
void setup_wifi() {
  Serial.println("Conectando ao WiFi...");
  WiFi.begin(ssid, password);

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFalha ao conectar no WiFi.");
  }
}

// ------------------ CALLBACK MQTT ------------------
void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) msg += (char)payload[i];

  Serial.println("-----------------------");
  Serial.print("Tópico: "); Serial.println(topic);
  Serial.print("Mensagem: "); Serial.println(msg);

  if (strcmp(topic, "MechBerry/Irrigacao/Acionar") == 0) {
    digitalWrite(MOTOR_PIN, LOW);
    Serial.println("Motor LIGADO via MQTT");
  }

  if (strcmp(topic, "MechBerry/Irrigacao/Desacionar") == 0) {
    digitalWrite(MOTOR_PIN, HIGH);
    Serial.println("Motor DESLIGADO via MQTT");
  }
}

// ------------------ CONECTAR MQTT ------------------
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT... ");

    if (client.connect("ESP32-MechBerry")) {
      Serial.println("Conectado!");
      client.subscribe(mqttTopicSub);
    } else {
      Serial.print("Falhou. Código=");
      Serial.print(client.state());
      Serial.println(" Tentando de novo em 2s...");
      delay(2000);
    }
  }
}

// ------------------ SETUP ------------------
void setup() {
  Serial.begin(115200);

  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, HIGH);

  pinMode(pinoLed, OUTPUT);
  digitalWrite(pinoLed, LOW);

  pinMode(ldrPin, INPUT);
  pinMode(pinoEletrodo, INPUT);

  pinMode(SENSOR_2, INPUT);
  pinMode(SENSOR_4, INPUT);

  setup_wifi();
  client.setServer(mqtt_server, mqttPort);
  client.setCallback(callback);
}

// ------------------ LOOP ------------------
void loop() {

  if (!client.connected()) reconnect();
  client.loop();

  // ===================== LEITURA A CADA 1 SEGUNDO =====================
  if (millis() - ultimoUpdate > intervaloLeitura) {
    ultimoUpdate = millis();

    Serial.println("\n===== Atualização de Sensores =====");

    // -------- LDR --------
    int valorLDR = analogRead(ldrPin);
    Serial.print("LDR: ");
    Serial.println(valorLDR);

    if (valorLDR > limiteLDR) digitalWrite(pinoLed, HIGH);
    else digitalWrite(pinoLed, LOW);

    // -------- ELETRODO (NÍVEL DE ÁGUA) --------
    int valorEletrodo = analogRead(pinoEletrodo);
    Serial.print("Nível de água (eletrodo): ");
    Serial.println(valorEletrodo);

    if (valorEletrodo < limiteAgua)
      Serial.println("⚠ Nível de água BAIXO!");
    else
      Serial.println("✔ Nível de água adequado.");

    // -------- UMIDADE DO SOLO --------
    int umid2 = calcularUmidade(analogRead(SENSOR_2), min_agua[0], max_ar[0]);
    int umid4 = calcularUmidade(analogRead(SENSOR_4), min_agua[1], max_ar[1]);

    Serial.print("Umidade Sensor 2: "); Serial.print(umid2); Serial.println("%");
    Serial.print("Umidade Sensor 4: "); Serial.print(umid4); Serial.println("%");

    Serial.println("=====================================");
  }
}
