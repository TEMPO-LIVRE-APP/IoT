#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <MPU6050.h>

// Wi-Fi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// MQTT HiveMQ
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_topic = "iot/solo/analisador123"; // personalize se quiser

WiFiClient espClient;
PubSubClient client(espClient);

// Sensores
#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
MPU6050 mpu;

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

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao HiveMQ...");
    if (client.connect("ESP32Client-001")) {
      Serial.println("conectado ao HiveMQ.");
    } else {
      Serial.print("falha, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // SDA, SCL
  dht.begin();
  mpu.initialize();

  setup_wifi();

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

  // Monta JSON
  String payload = "{";
  payload += "\"temperatura\":" + String(temperatura) + ",";
  payload += "\"umidade\":" + String(umidade) + ",";
  payload += "\"ax\":" + String(ax) + ",";
  payload += "\"ay\":" + String(ay) + ",";
  payload += "\"az\":" + String(az);
  payload += "}";

  // Envia ao broker HiveMQ
  client.publish(mqtt_topic, payload.c_str());

  Serial.println("Dados enviados via MQTT:");
  Serial.println(payload);
  Serial.println("------");

  delay(5000); // Aguarda 5 segundos
}
