#include <FastLED.h>
#include <M5Unified.h> // M5Unifiedライブラリをプログラムで使用可能にします。

// グローバル変数（プログラム全体で使用する変数の定義をします。）
constexpr int PIN_LED = 21; // 本体フルカラーLEDの使用端子（G21）
constexpr int NUM_LEDS = 1; // 本体フルカラーLEDの数

CRGB led{};

void setup() {
  auto cfg = M5.config(); // M5Stack初期設定用の構造体を代入
  M5.begin(cfg);          // M5デバイスの初期化

  FastLED.addLeds<WS2812B, PIN_LED, GRB>(&led, NUM_LEDS);
  led =
      CRGB(40, 40, 40); // 白色（赤, 緑, 青）※3色それぞれの明るさを0〜255で指定
}

// 赤 -> 消える -> 白を繰り返す
void loop() {
  // Turn the LED on, then pause
  led = CRGB::Red;
  FastLED.show();
  delay(500);

  // Now turn the LED off, then pause
  led = CRGB::Black;
  FastLED.show();
  delay(500);

  // Turn the LED on, then pause
  led = CRGB(40, 40, 40);
  FastLED.show();
  delay(500);
}
