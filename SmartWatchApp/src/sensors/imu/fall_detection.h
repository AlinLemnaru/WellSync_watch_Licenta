#pragma once

void initFallDetection();
void updateFallDetection();

bool isFallConfirmationActive();
bool isFallAlertActive();
bool isFallOverlayActive();

void confirmUserIsOk();
void triggerFallDetectedForTest();