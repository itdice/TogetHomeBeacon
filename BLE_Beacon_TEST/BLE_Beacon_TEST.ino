#include <time.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEBeacon.h>
#include <esp_sleep.h>

#define DEEP_SLEEP_DURATION 1
RTC_DATA_ATTR static time_t last; // Remember last boot in RTC Memory
struct timeval now; // Time Struct

BLEAdvertising *pAdvertising; // BLE Advertisement type

#define BEACON_UUID "f5c3d5bb-2196-43ea-a5f9-6206c7c30483" // UUID

#define STATE_LED 13
#define BAT_CHECK 34

void setBeacon() {
  BLEBeacon oBeacon = BLEBeacon();

  oBeacon.setManufacturerId(0x004C); // Manufacture ID
  oBeacon.setProximityUUID(BLEUUID(BEACON_UUID)); // Set Proximirty UUID

  oBeacon.setMajor(0x1000); // Major ID = Identification between Beacons
  oBeacon.setMinor(0x1000); // Minor ID = State(4bits) + Battery Level(4bits) + RSSI at 1m(8bits)

  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
  BLEAdvertisementData oScanResponseData = BLEAdvertisementData();

  oAdvertisementData.setFlags(0x04); // Basic Rate, Enhanced Data Rate NOT Supported

  std::string strServiceData = "";
  strServiceData += (char)26; // Length
  strServiceData += (char)0xFF; // Type

  strServiceData + oBeacon.getData();
  oAdvertisementData.addData(strServiceData);
  pAdvertising->setAdvertisementData(oAdvertisementData);
  pAdvertising->setScanResponseData(oScanResponseData);
}

void setup() {
  Serial.begin(115200); // Serial OPEN
  gettimeofday(&now, NULL); // Time

  pinMode(STATE_LED, OUTPUT);
  int batRAW = analogRead(BAT_CHECK); // Check battery level through Voltage

  Serial.printf("Start Toget-Home Beacon\n");
  Serial.printf("Deep Sleep (%lds since last reset, %lds since last boot)\n", now.tv_sec, now.tv_sec - last);
  Serial.printf("Analog Value : %d\n", batRAW);

  last = now.tv_sec;

  BLEDevice::init("Toget Home"); // Crate BLE Device
  BLEServer *pServer = BLEDevice::createServer(); // Create BLE Server
  pAdvertising = BLEDevice::getAdvertising();
  BLEDevice::startAdvertising();
  setBeacon(); // Beacon Setting

  pAdvertising->start(); // STart Advertising
  Serial.printf("Advertising Started...\n");
  digitalWrite(STATE_LED, HIGH);
  delay(100); // Start Delay
  digitalWrite(STATE_LED, LOW);
  pAdvertising->stop();

  Serial.printf("Enter Deep Sleep\n");
  esp_deep_sleep(1000000LL * DEEP_SLEEP_DURATION);
  Serial.printf("in Deep Sleep...\n");
}

void loop() {
}
