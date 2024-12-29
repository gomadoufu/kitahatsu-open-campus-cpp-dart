
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <FastLED.h>
#include <M5Unified.h>

// グローバル変数（プログラム全体で使用する変数の定義をします。）
constexpr int PIN_LED = 21; // 本体フルカラーLEDの使用端子（G21）
constexpr int NUM_LEDS = 1; // 本体フルカラーLEDの数

CRGB led;

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) { deviceConnected = true; };

  void onDisconnect(BLEServer *pServer) { deviceConnected = false; }
};

void setup_log() {
  M5.Log.setLogLevel(m5::log_target_serial, ESP_LOG_DEBUG);
  M5.Log.setEnableColor(m5::log_target_serial, true);
}

void setup_ble() {
  // Create the BLE Device
  BLEDevice::init("kitahatsu-open-campus");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ |
                               BLECharacteristic::PROPERTY_WRITE |
                               BLECharacteristic::PROPERTY_NOTIFY |
                               BLECharacteristic::PROPERTY_INDICATE);

  // Creates BLE Descriptor 0x2902: Client Characteristic Configuration
  // Descriptor (CCCD)
  pCharacteristic->addDescriptor(new BLE2902());
  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(
      0x0); // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
}

void setup_fast_led() {
  FastLED.addLeds<WS2812B, PIN_LED, GRB>(&led, NUM_LEDS);
  led =
      CRGB(40, 40, 40); // 白色（赤, 緑, 青）※3色それぞれの明るさを0〜255で指定
}

void setup() {
  auto cfg = M5.config(); // M5Stack初期設定用の構造体を代入
  M5.begin(cfg);          // M5デバイスの初期化
  setup_fast_led();
  setup_log();
  setup_ble();
  M5_LOGD("Waiting a client connection to notify...");
}

void loop() {
  M5.update();
  // notify changed value
  if (deviceConnected) {
    led = CRGB::Green;
    pCharacteristic->setValue((uint8_t *)&value, 4);
    pCharacteristic->notify();
    value++;
    delay(500);
  }
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    led = CRGB::Red;
    delay(500); // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    M5_LOGD("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
    led = CRGB(40, 40, 40);
  }
  FastLED.show();
}
