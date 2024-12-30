#pragma once
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

namespace goma {
BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) { deviceConnected = true; };

  void onDisconnect(BLEServer *pServer) { deviceConnected = false; }
};

class Ble_Controller {
  const std::string DEVICE_NAME = "kitahatsu";
  // See the following for generating UUIDs:
  // https://www.uuidgenerator.net/
  const std::string SERVICE_UUID = "c39e46c6-88d8-48d2-9310-278848445900";
  const std::string CHARACTERISTIC_UUID =
      "68d37433-bd72-4ee1-95e0-b9f1d1e27dce";

public:
  void setup() {
    // Create the BLE Device
    BLEDevice::init(DEVICE_NAME);

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

  void send() {

    // 送信したい文字列（UTF-8）
    std::string message = u8"沸いてるよー";
    pCharacteristic->setValue(message);
    pCharacteristic->notify();
  }
};

} // namespace goma
