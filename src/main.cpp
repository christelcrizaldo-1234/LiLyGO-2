#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <Adafruit_SHT31.h>

Adafruit_SHT31 sht;


uint8_t HELTEC1[] = {0x24,0x58,0x7C,0x5B,0x3C,0xA8};
uint8_t NODE1[]   = {0x44,0x17,0x93,0xE4,0xDE,0x90};
uint8_t NODE3[]   = {0xC8,0x2E,0x18,0xAC,0x52,0x34};

typedef struct {
  int node_id;
  float soil_moisture;
  float soil_temperature;
  float conductivity;
  float ph;
  float air_temperature;
  float humidity;
  float distance;
  uint8_t hop;
  uint8_t source_id;
} SensorData;

SensorData data;
SensorData n1, n3;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
void OnRecv(const esp_now_recv_info_t *info, const uint8_t *d, int len) {
#else
void OnRecv(const uint8_t *mac, const uint8_t *d, int len) {
#endif
  SensorData in;
  memcpy(&in, d, sizeof(in));

  if (in.node_id == 1) n1 = in;
  if (in.node_id == 3) n3 = in;
}

void sendData() {
  if (esp_now_send(HELTEC1, (uint8_t*)&data, sizeof(data)) != ESP_OK) {
    esp_now_send(NODE1, (uint8_t*)&data, sizeof(data));
    esp_now_send(NODE3, (uint8_t*)&data, sizeof(data));
  }
  Serial.println("NODE 2 SENT");
}

void setup() {
  Serial.begin(115200);

  Wire.begin();
  sht.begin(0x44);

  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_recv_cb(OnRecv);

  esp_now_peer_info_t p{};
  memcpy(p.peer_addr, HELTEC1, 6); esp_now_add_peer(&p);
  memcpy(p.peer_addr, NODE1,   6); esp_now_add_peer(&p);
  memcpy(p.peer_addr, NODE3,   6); esp_now_add_peer(&p);

  data.node_id = 2;
  data.source_id = 2;
}

void loop() {
  data.air_temperature  = sht.readTemperature();
  data.humidity         = sht.readHumidity();

  data.soil_moisture    = 0;
  data.soil_temperature = 0;
  data.conductivity     = 0;
  data.ph               = 0;
  data.distance         = 0;
  data.hop              = 0;

  sendData();

  Serial.println("\n===== NODE 2 VIEW =====");
  Serial.print("Air Temp: "); Serial.println(data.air_temperature);
  Serial.print("Humidity: "); Serial.println(data.humidity);
  Serial.println("======================");

  delay(4000);
}

//END