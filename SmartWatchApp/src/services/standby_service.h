#pragma once

void initStandby();
void updateStandby();
void notifyUserActivity();
void wakeFromStandby();
void wakeForAlert();             
void restoreNormalBrightness();  
bool isStandbyActive();