#pragma once
#include "app_types.h"

extern AppState app;

void initAppState();

const char* getPageTitle(PageId page);
const char* getConnectionLabel(ConnectionState state);
void updateBatteryState();