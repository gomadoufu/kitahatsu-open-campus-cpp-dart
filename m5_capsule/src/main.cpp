#include "Recorder.hpp"
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <FastLED.h>
#include <M5Unified.h>

// グローバル変数（プログラム全体で使用する変数の定義をします。）
constexpr int PIN_LED = 21; // 本体フルカラーLEDの使用端子（G21）
constexpr int NUM_LEDS = 1; // 本体フルカラーLEDの数

constexpr int32_t VOLUME_THRESHOLD = 3000;
gomadoufu::Recorder goma_recorder{};

CRGB led{};

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;

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

  // LEDを赤にしておく
  led = CRGB::Red;
  FastLED.show();

  goma_recorder.setup();

  // マイクの初期化
  M5.Mic.begin();
  M5_LOGD("begin mic");
}

void loop() {
  M5.update();

  goma_recorder.record();
  auto maxValue = goma_recorder.get_peak();
  M5_LOGD("Recorded: peak = %d", maxValue);

  // notify changed value
  if (deviceConnected) {
    if (maxValue > VOLUME_THRESHOLD) {
      led = CRGB::Green;
      pCharacteristic->setValue((uint8_t *)&maxValue, 2);
      pCharacteristic->notify();
    } else {
      led = CRGB::Red;
    }
  }
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // give the bluetooth stack the chance to get things ready
    led = CRGB(40, 40, 40);
    pServer->startAdvertising(); // restart advertising
    M5_LOGD("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
    led = CRGB::Blue;
  }
  FastLED.show();
}
