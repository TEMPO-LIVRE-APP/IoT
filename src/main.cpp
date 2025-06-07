#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <MPU6050.h>
#include <time.h>

// Cords Location
const float LATITUDE = -23.5505;
const float LONGITUDE = -46.6333;

// Wi-Fi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// MQTT HiveMQ
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_topic = "iot/esp32/solo"; // tópico único para todos

WiFiClient espClient;
PubSubClient client(espClient);

// Sensores
#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
MPU6050 mpu;

// Função para conectar ao Wi-Fi
void setup_wifi() {
  Serial.println("Conectando ao Wi-Fi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado. Endereço IP: ");
  Serial.println(WiFi.localIP());
}

// Conexão com o broker MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao HiveMQ...");
    String clientId = "ESP32Client-" + WiFi.macAddress();
    if (client.connect(clientId.c_str())) {
      Serial.println("conectado ao HiveMQ.");
    } else {
      Serial.print("falha, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

// Configura fuso e sincroniza horário NTP
void setup_time() {
  configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov"); // GMT-3
  Serial.println("Aguardando sincronização do horário...");
  while (time(nullptr) < 100000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nHorário sincronizado.");
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // SDA, SCL
  dht.begin();
  mpu.initialize();

  setup_wifi();
  setup_time();

  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Leitura dos sensores
  float temperatura = dht.readTemperature();
  float umidade = dht.readHumidity();

  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);

  // Validação de leitura
  if (isnan(temperatura)) temperatura = -273.15;
  if (isnan(umidade)) umidade = 0;

  // Obter horário atual formatado
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  char horario[20];
  strftime(horario, sizeof(horario), "%d/%m/%y %H:%M:%S", timeinfo);

  // Obter MAC address
  String mac = WiFi.macAddress();

  // Monta JSON com campo "id" e "horario"
  String payload = "{";
  payload += "\"id\":\"" + mac + "\",";
  payload += "\"horario\":\"" + String(horario) + "\",";
  payload += "\"temperatura\":" + String(temperatura) + ",";
  payload += "\"umidade\":" + String(umidade) + ",";
  payload += "\"ax\":" + String(ax) + ",";
  payload += "\"ay\":" + String(ay) + ",";
  payload += "\"az\":" + String(az) + ",";
  payload += "\"latitude\":" + String(LATITUDE, 6) + ",";
  payload += "\"longitude\":" + String(LONGITUDE, 6);
  payload += "}";

  // Publica no broker
  client.publish(mqtt_topic, payload.c_str());

  Serial.println("Dados enviados via MQTT:");
  Serial.println(payload);
  Serial.println("------");

  delay(5000); // intervalo entre leituras
}
