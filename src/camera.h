// src/camera.h — Cámara de batalla (zoom, seguimiento, interpolación)
#pragma once
#include "config.h"

// Extraído de initBattle (:1458-1463) y updateDrawBattle (:2471-2500)
void initBattleCamera();
void updateBattleCamera(float dt);
