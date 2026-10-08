// src/fx.h — VFX: partículas de combate (Fase 4)
#pragma once

#include "raylib.h"

void fxClear();                          // limpiar pool (initBattle)
void fxSpawnSparks(Vector2 pos,int n);   // chispas de impacto (golpes metálicos)
void fxSpawnBlood(Vector2 pos,int n);    // sangre sutil (daño/muerte)
void fxBattleDust(float dt);             // polvo de marcha (recorre unidades vivas)
void fxUpdate(float dt);                 // dt ya escalado (timeScale); no llamar en pausa
void fxDraw();                           // dentro de beginBattleView/endBattleView
