#include <M5Unified.h>
#include "ui_splash.h"
#include "ui_theme.h"

void drawSplashScreen() {
    const int W = 320;
    const int H = 240;

    // WellSync colors
    const uint16_t COLOR_WELL = 0x2BB0;  // #297481 — dark teal
    const uint16_t COLOR_SYNC = 0x7574;  // #77AEA0 — light teal 

    M5.Display.fillScreen(TFT_BLACK);

    UITheme::applyFontClock(M5.Display);

    int wellW = M5.Display.textWidth("Well");
    int syncW = M5.Display.textWidth("Sync");

    int totalW  = wellW + syncW;
    int startX  = (W - totalW) / 2;
    int centerY = (H / 2) - (M5.Display.fontHeight() / 2);

    // "Well"
    M5.Display.setTextDatum(TL_DATUM);
    M5.Display.setTextColor(COLOR_WELL, TFT_BLACK);
    M5.Display.drawString("Well", startX, centerY);

    // "Sync"
    M5.Display.setTextColor(COLOR_SYNC, TFT_BLACK);
    M5.Display.drawString("Sync", startX + wellW, centerY);
}