#pragma once

// Short audible feedback for key presses. No-op when BUZZER_PIN < 0.
void beep(int freq, int duration);
