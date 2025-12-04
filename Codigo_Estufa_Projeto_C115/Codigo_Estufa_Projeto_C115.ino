// === BLYNK ===
#define BLYNK_TEMPLATE_ID "TMPL2EHUkuYqb"
#define BLYNK_TEMPLATE_NAME "Estufa"
#define BLYNK_AUTH_TOKEN "Kx9emv2mI-1yOLQ5GenOLfcmHUXt7H9d"

#include <WiFi.h>
#include <PubSubClient.h>
#include <BlynkSimpleEsp32.h>

// === PINAGEM ===
#define MOTOR_PIN 25
const int pinoLed = 26;
const int ldrPin = 32;
const int pinoEletrodo = 35;

#define SENSOR_2 34
#define SENSOR_4 39

// === CONFIG WI-FI ===
const char* ssid = "WLL-Inatel";
const char* password = "inatelsemfio";

// === CONFIG MQTT ===
const char* mqtt_server = "test.mosquitto.org";
const int mqttPort = 1883;
const char* mqttTopicSub = "Estufa/Irrigacao/#";

WiFiClient espClient;
PubSubClient client(espClient);

// === VARIÁVEIS ===
int limiteLDR = 800;
int limiteAgua = 500;
int min_agua[] = {1469, 1419};
int max_ar[]   = {3575, 3438};

unsigned long ultimoUpdate = 0;
const unsigned long intervaloLeitura = 1000;

// === FUNÇÃO DE UMIDADE ===
int calcularUmidade(int valor_lido, int min_val, int max_val) {
  int umid = map(valor_lido, min_val, max_val, 100, 0);
  if (umid > 100) umid = 100;
  if (umid < 0) umid = 0;
  return umid;
}

// === WIFI ===
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
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFalha ao conectar no WiFi.");
  }
}

// === MQTT CALLBACK ===
void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) msg += (char)payload[i];

  Serial.println("-----------------------");
  Serial.print("Tópico: "); Serial.println(topic);
  Serial.print("Mensagem: "); Serial.println(msg);

  if (strcmp(topic, "Estufa/Irrigacao/Acionar") == 0) {
    digitalWrite(MOTOR_PIN, LOW);
    Serial.println("Motor LIGADO via MQTT");
    Blynk.virtualWrite(V1, 1);
  }

  if (strcmp(topic, "Estufa/Irrigacao/Desacionar") == 0) {
    digitalWrite(MOTOR_PIN, HIGH);
    Serial.println("Motor DESLIGADO via MQTT");
    Blynk.virtualWrite(V1, 0);
  }
}

// === MQ CONNECT ===
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT... ");
    if (client.connect("ESP32-Estufa")) {
      Serial.println("Conectado!");
      client.subscribe(mqttTopicSub);
    } else {
      Serial.print("Falhou. RC=");
      Serial.print(client.state());
      Serial.println(" Tentando de novo...");
      delay(2000);
    }
  }
}

// === CONTROLE DO MOTOR PELO BLYNK ===
BLYNK_WRITE(V1) {
  int valor = param.asInt();
  if (valor == 1) {
    digitalWrite(MOTOR_PIN, LOW);
    Serial.println("Motor ligado via Blynk");
  } else {
    digitalWrite(MOTOR_PIN, HIGH);
    Serial.println("Motor desligado via Blynk");
  }
}

// === SETUP ===
void setup() {
  Serial.begin(115200);

  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, HIGH);

  pinMode(pinoLed, OUTPUT);
  pinMode(ldrPin, INPUT);
  pinMode(pinoEletrodo, INPUT);
  pinMode(SENSOR_2, INPUT);
  pinMode(SENSOR_4, INPUT);

  setup_wifi();
  client.setServer(mqtt_server, mqttPort);
  client.setCallback(callback);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
}

// === LOOP ===
void loop() {
  Blynk.run();

  if (!client.connected()) reconnect();
  client.loop();

  if (millis() - ultimoUpdate > intervaloLeitura) {
    ultimoUpdate = millis();

    int valorLDR = analogRead(ldrPin);
    int valorEletrodo = analogRead(pinoEletrodo);
    int umid2 = calcularUmidade(analogRead(SENSOR_2), min_agua[0], max_ar[0]);
    int umid4 = calcularUmidade(analogRead(SENSOR_4), min_agua[1], max_ar[1]);

    Serial.println("\n===== Sensores =====");
    Serial.print("LDR: "); Serial.println(valorLDR);
    Serial.print("Água: "); Serial.println(valorEletrodo);
    Serial.print("Umid2: "); Serial.println(umid2);
    Serial.print("Umid4: "); Serial.println(umid4);

    // === CONTROLE DO LED PELO LDR ===
    if (valorLDR > limiteLDR) {
      digitalWrite(pinoLed, HIGH);
      Serial.println("LED DESLIGADO (muita luz)");
      
    } else {
      digitalWrite(pinoLed, LOW);
      Serial.println("LED LIGADO (pouca luz)");
    }

    // ENVIO PARA O BLYNK
    Blynk.virtualWrite(V8, valorEletrodo);
    Blynk.virtualWrite(V5, valorLDR);
    Blynk.virtualWrite(V6, umid2);
    Blynk.virtualWrite(V7, umid4);
  }
}
