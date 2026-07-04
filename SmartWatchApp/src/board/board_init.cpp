#include <M5Unified.h>
#include "board_init.h"

void initBoard() {
  auto cfg = M5.config();
  cfg.clear_display = true;
  M5.begin(cfg);

  M5.Display.setRotation(1);
  M5.Display.fillScreen(TFT_BLACK);
  delay(40);
  M5.Display.fillScreen(TFT_BLACK);
  delay(40);

  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);

  Serial.begin(115200);
  Serial.println("Board initialized");
}