#include "../app/app_state.h"
#include "ui_pages.h"
#include "pages/page_home.h"
#include "pages/page_health.h"
#include "pages/page_environment.h"
#include "pages/page_sleep.h"
#include "pages/page_stress.h"
#include "pages/page_settings.h"

void drawActivePage(M5Canvas& canvas) {
  switch (app.currentPage) {
    case PAGE_HOME:
      drawPageHome(canvas);
      break;

    case PAGE_HEALTH:
      drawPageHealth(canvas);
      break;

    case PAGE_STRESS:
      drawPageStress(canvas);
      break;

    case PAGE_ENVIRONMENT:
      drawPageEnvironment(canvas);
      break;

    case PAGE_SLEEP:
      drawPageSleep(canvas);
      break;

    case PAGE_SETTINGS:
      drawPageSettings(canvas);
      break;

    default:
      drawPagePlaceholder(canvas);
      break;
  }
}