#include <time.h>
#include <esp_sleep.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <Arduino.h>
#include <eddystone.hpp>

#define DEEP_SLEEP_DURATION 5
#define STATE_LED 32
#define TRIG_LED 33
#define TRIG_CHECK 34
#define BAT_CHECK 35

#define STATE_INITIAL 0xFF
#define STATE_NORMAL 0x00
#define STATE_TRIGGERED 0x10
#define STATE_LOW_BATTERY 0x20
#define STATE_CHARGED 0x30

uint8_t currentState = STATE_INITIAL; // initial state
uint8_t batteryLevel = 0x00; // initial battery level

RTC_DATA_ATTR static time_t last; // Remember last boot in RTC Memory
struct timeval now; // Time Struct

const float maxBatteryVoltage = 1.9;
const float minBatteryVoltage = 1.5;

const std::string NAMESPACE = "17FD1CEFFF705E7F803E"; // Beacon Group ID "IT*DICE***TOGET*HOME"
const std::string INSTANCE  = "DE248BAD97AF"; // Beacon Characteristic ID (Random)

void setBeacon(BLEAdvertising* pAdvertising) {
  EddystoneUid uid(NAMESPACE, INSTANCE);
  uid.setRSSI(0xC2); // RSSI at 1M (-62dBm)
  uid.setState(currentState, batteryLevel);
  BLEAdvertisementData adData;
  uid.compose(adData);
  pAdvertising->setAdvertisementData(adData);
}

void setup() {
  // Serial OPEN
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  
  // Initial SET
  gettimeofday(&now, NULL); // Time
  
  pinMode(STATE_LED, OUTPUT);
  pinMode(TRIG_LED, OUTPUT);
  pinMode(TRIG_CHECK, INPUT);
  pinMode(BAT_CHECK, INPUT);

  int trig = digitalRead(TRIG_CHECK); // Check Trigger Button
  int batRAW = analogRead(BAT_CHECK); // Check battery level through Voltage
  float batVOLT = (batRAW / 4095.0) * 3.3;
  float clampBatVOLT = constrain(batVOLT, minBatteryVoltage, maxBatteryVoltage);
  int batLV = int(((clampBatVOLT - minBatteryVoltage) / (maxBatteryVoltage - minBatteryVoltage)) * 10) * 10;

  Serial.printf("Start Toget-Home Beacon\n");
  Serial.printf("Deep Sleep (%lds since last reset, %lds since last boot)\n", now.tv_sec, now.tv_sec - last);
  Serial.printf("Analog Value : %d\n", batRAW);
  Serial.printf("Battery Level : %d\n", batLV);
  Serial.printf("Trigger Value : %d\n", trig);
  last = now.tv_sec;

  // BLE Start
  BLEDevice::init("Toget Home");
  BLEServer *pServer = BLEDevice::createServer();
  BLEAdvertising *pAdvertising = pServer->getAdvertising();

  batteryLevel = (uint8_t)(batLV);

  if((batteryLevel > 20) && (trig == LOW)) {
    currentState = STATE_NORMAL;
  }
  else if((batteryLevel <= 20) && (trig == LOW)) {
    currentState = STATE_LOW_BATTERY;
  }
  else {
    currentState = STATE_TRIGGERED;
    digitalWrite(TRIG_LED, HIGH);
  }

  setBeacon(pAdvertising);
  Serial.println("Advertising Started...\n");
  pAdvertising->start();

  digitalWrite(STATE_LED, HIGH);
  delay(1000); // Start Delay
  digitalWrite(STATE_LED, LOW);
  digitalWrite(TRIG_LED, LOW);

  pAdvertising->stop();
  Serial.printf("Enter Deep Sleep\n");
  esp_deep_sleep(1000000LL * DEEP_SLEEP_DURATION);
  Serial.printf("in Deep Sleep...\n");
}

void loop() {
}
