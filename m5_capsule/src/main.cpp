#include "Ble_Controller.hpp"
#include "Capsule_Led.hpp"
#include "Recorder.hpp"
#include <M5Unified.h>
#include <string>

constexpr int32_t VOLUME_THRESHOLD = 3000;
goma::Recorder goma_recorder{};
goma::Ble_Controller goma_ble_controller{};

void setup_log() {
  M5.Log.setLogLevel(m5::log_target_serial, ESP_LOG_DEBUG);
  M5.Log.setEnableColor(m5::log_target_serial, true);
}

void setup() {
  auto cfg = M5.config(); // M5Stack初期設定用の構造体を代入
  M5.begin(cfg);          // M5デバイスの初期化
  setup_log();

  M5_LOGD("Waiting a client connection to notify...");

  goma::setup_fast_led();
  goma_recorder.setup();
  goma_ble_controller.setup();

  goma::led_red();
}

void loop() {
  M5.update();

  goma_recorder.record();

  auto maxValue = goma_recorder.get_peak();
  M5_LOGD("Recorded: peak = %d", maxValue);

  // notify changed value
  if (goma::deviceConnected) {
    if (maxValue > VOLUME_THRESHOLD) {
      goma::led_green();
      goma_ble_controller.send();
    } else {
      goma::led_black();
    }
  }

  // disconnecting
  if (!goma::deviceConnected && goma::oldDeviceConnected) {
    goma::led_blue();
    delay(500); // give the bluetooth stack the chance to get things ready
    goma::pServer->startAdvertising(); // restart advertising
    M5_LOGD("start advertising");
    goma::oldDeviceConnected = goma::deviceConnected;
  }

  // connecting
  if (goma::deviceConnected && !goma::oldDeviceConnected) {
    // do stuff here on connecting
    goma::oldDeviceConnected = goma::deviceConnected;
    goma::led_blue();
  }
}
