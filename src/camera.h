// src/camera.h — Cámara de batalla (zoom, seguimiento, interpolación)
#pragma once
#include "config.h"

// Extraído de initBattle (:1458-1463) y updateDrawBattle (:2471-2500)
void initBattleCamera();
void updateBattleCamera(float dt);

// Proyección de la vista de batalla. Fase 0: bit-equivalente a Camera2D
// (offset/target/zoom, rotation=0). Fase 2 (isometría): la rotación 45° +
// squash Y entran SOLO aquí (beginBattleView/endBattleView + las helpers).
void beginBattleView();
void endBattleView();
Vector2 worldToScreenBattle(Vector2 world);
Vector2 screenToWorldBattle(Vector2 screen);
// Rect visible del mundo en la convención histórica del minimapa
// (ancho = SCREEN_W/zoom, alto = (SCREEN_H-HUD_H)/zoom, esquina = target-dim/2).
void battleViewRect(float& x,float& y,float& w,float& h);
