#include "Recorder.hpp"
#include <FastLED.h>
#include <M5Unified.h>

CRGB led{};

// LED制御用ピンとLED数
constexpr int PIN_LED = 21;
constexpr int NUM_LEDS = 1;

constexpr int32_t VOLUME_THRESHOLD = 3000;
gomadoufu::Recorder goma_recorder{};

void setup_log() {
  M5.Log.setLogLevel(m5::log_target_serial, ESP_LOG_DEBUG);
  M5.Log.setEnableColor(m5::log_target_serial, true);
}

void setup(void) {
  auto cfg = M5.config();
  M5.begin(cfg);
  setup_log();

  // LED初期化
  FastLED.addLeds<WS2812B, PIN_LED, GRB>(&led, NUM_LEDS);

  // LEDを赤にしておく
  led = CRGB::Red;
  FastLED.show();

  goma_recorder.setup();

  // マイクの初期化
  M5.Mic.begin();
  M5_LOGD("begin mic");
}

void loop(void) {
  M5.update();

  goma_recorder.record();
  auto maxValue = goma_recorder.get_peak();

  // ピーク値が閾値を超えているかどうかでLEDの色を切り替え
  if (maxValue > VOLUME_THRESHOLD) {
    led = CRGB::Green; // 音量大きい場合
  } else {
    led = CRGB::Red; // 音量小さい場合
  }
  FastLED.show();

  // デバッグ用にログ出力（最初のサンプル値と最大振幅）
  M5_LOGD("Recorded: peak = %d", maxValue);
}
