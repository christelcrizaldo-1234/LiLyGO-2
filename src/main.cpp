```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <Adafruit_SHT31.h>

#define BOARD_POWERON_PIN 12

// ================= BATTERY =================
#define BAT_ADC_PIN       35
#define ADC_REF_VOLTAGE   3.3f
#define ADC_RESOLUTION    4095.0f
#define VOLTAGE_DIVIDER   2.0f

Adafruit_SHT31 sht;

// MAC Addresses
uint8_t HELTEC1[] = {0x24,0x58,0x7C,0x5B,0x3C,0xA8};
uint8_t NODE1[]   = {0x44,0x17,0x93,0xE4,0xDE,0x90};
uint8_t NODE3[]   = {0xC8,0x2E,0x18,0xAC,0x52,0x34};

// =====================================================
// DATA STRUCTURE
// MUST MATCH HELTEC1 / HELTEC2 / NODE3
// =====================================================
typedef struct {
  int node_id;

  float soil_moisture;
  float soil_temperature;
  float conductivity;
  float ph;

  float air_temperature;
  float humidity;

  float ultrasonic_distance;   // Node3 sensor distance
  float espnow_distance;       // RSSI distance to Heltec1

  float battery_voltage;

  uint8_t hop;
  uint8_t source_id;
} SensorData;

SensorData data;
SensorData n1, n3;

// ================= BATTERY FUNCTION =================
float getBatteryVoltage() {
  int raw = analogRead(BAT_ADC_PIN);

  return (raw / ADC_RESOLUTION)
         * ADC_REF_VOLTAGE
         * VOLTAGE_DIVIDER;
}

// =====================================================
// RECEIVE CALLBACK
// =====================================================
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
void OnRecv(const esp_now_recv_info_t *info,
            const uint8_t *d,
            int len)
#else
void OnRecv(const uint8_t *mac,
            const uint8_t *d,
            int len)
#endif
{
  if (len != sizeof(SensorData)) return;

  SensorData in;
  memcpy(&in, d, sizeof(in));

  if (in.node_id == 1) n1 = in;
  if (in.node_id == 3) n3 = in;
}

// =====================================================
// SEND DATA
// =====================================================
void sendData() {

  esp_err_t result =
      esp_now_send(HELTEC1,
                   (uint8_t *)&data,
                   sizeof(data));

  if (result != ESP_OK) {

    esp_now_send(NODE1,
                 (uint8_t *)&data,
                 sizeof(data));

    esp_now_send(NODE3,
                 (uint8_t *)&data,
                 sizeof(data));
  }

  Serial.println("NODE 2 SENT");
}

// =====================================================
// SETUP
// =====================================================
void setup() {

  Serial.begin(115200);

  pinMode(BOARD_POWERON_PIN, OUTPUT);
  digitalWrite(BOARD_POWERON_PIN, HIGH);

  delay(1000);

  // Battery ADC
  pinMode(BAT_ADC_PIN, INPUT);
  analogSetAttenuation(ADC_11db);

  // I2C
  Wire.begin();

  // SHT31
  if (!sht.begin(0x44)) {
    Serial.println("SHT31 NOT FOUND");
    while (1);
  }

  Serial.println("SHT31 OK");

  // WiFi
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW INIT FAILED");
    while (1);
  }

  esp_now_register_recv_cb(OnRecv);

  // HELTEC1
  esp_now_peer_info_t peerInfo = {};

  memcpy(peerInfo.peer_addr, HELTEC1, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
    Serial.println("Failed to add HELTEC1");

  // NODE1
  memcpy(peerInfo.peer_addr, NODE1, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
    Serial.println("Failed to add NODE1");

  // NODE3
  memcpy(peerInfo.peer_addr, NODE3, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
    Serial.println("Failed to add NODE3");

  data.node_id   = 2;
  data.source_id = 2;

  Serial.println("NODE 2 READY");
}

// =====================================================
// LOOP
// =====================================================
void loop() {

  digitalWrite(BOARD_POWERON_PIN, HIGH);

  // Read SHT31
  data.air_temperature = sht.readTemperature();
  data.humidity        = sht.readHumidity();

  // Battery
  data.battery_voltage = getBatteryVoltage();

  // Unused fields
  data.soil_moisture       = 0;
  data.soil_temperature    = 0;
  data.conductivity        = 0;
  data.ph                  = 0;

  data.ultrasonic_distance = 0;
  data.espnow_distance     = 0;

  data.hop                 = 0;

  sendData();

  Serial.println("\n===== NODE 2 VIEW =====");

  Serial.print("Air Temp: ");
  Serial.print(data.air_temperature);
  Serial.println(" °C");

  Serial.print("Humidity: ");
  Serial.print(data.humidity);
  Serial.println(" %");

  Serial.print("Battery: ");
  Serial.print(data.battery_voltage, 2);
  Serial.println(" V");

  Serial.print("ESP-NOW Distance: ");
  Serial.print(data.espnow_distance, 2);
  Serial.println(" m");

  Serial.println("======================");

  delay(4000);
}
