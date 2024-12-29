#pragma once

#include <M5Unified.h>

namespace gomadoufu {

class Recorder {
  const size_t record_number = 256;
  const size_t record_length = 200;
  const size_t record_size = record_number * record_length;
  const size_t record_samplerate = 44100;
  size_t rec_record_idx = 0;
  int16_t *rec_data = nullptr;
  bool success = false;

public:
  // 録音バッファの確保
  auto setup() -> void {
    rec_data = (int16_t *)heap_caps_malloc(record_size * sizeof(int16_t),
                                           MALLOC_CAP_8BIT);
    if (rec_data == nullptr) {
      M5_LOGE("Failed to allocate memory for rec_data");
    } else {
      memset(rec_data, 0, record_size * sizeof(int16_t));
    }
  }

  auto record() -> bool {
    if (!M5.Mic.isEnabled() ||
        rec_data == nullptr) { // マイクが無効 or
                               // バッファが確保されていなければ何もしない
      M5_LOGE("Mic is not enabled");
      return false;
    }
    auto data = &rec_data[rec_record_idx * record_length];
    success = M5.Mic.record(data, record_length, record_samplerate);
    if (success) {
      rec_record_idx++;
      if (rec_record_idx >= record_number) {
        rec_record_idx = 0;
      }
    }
    return success;
  }

  auto get_peak() -> int16_t {
    if (!success) {
      return 0;
    }
    int16_t maxValue = 0;
    auto data = &rec_data[rec_record_idx * record_length];
    for (int i = 0; i < record_length; i++) {
      int16_t absVal = abs(data[i]);
      if (absVal > maxValue) {
        maxValue = absVal;
      }
    }
    return maxValue;
  }
};

} // namespace gomadoufu
