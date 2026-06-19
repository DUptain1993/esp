#include "SoundUtils.h"
#include "Pins.h"
#include <Arduino.h>

// Non-blocking-ish piezo click. Disabled (compiled to a no-op) unless a real
// BUZZER_PIN is assigned in Pins.h. The CYD 3.5" ships without a buzzer.
void beep(int freq, int duration) {
#if (BUZZER_PIN >= 0)
  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    tone(BUZZER_PIN, freq, duration);
  #else
    ledcSetup(15, freq, 8);
    ledcAttachPin(BUZZER_PIN, 15);
    ledcWriteTone(15, freq);
    delay(duration);
    ledcWriteTone(15, 0);
    ledcDetachPin(BUZZER_PIN);
  #endif
#else
  (void)freq;
  (void)duration;
#endif
}
