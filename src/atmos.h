// src/atmos.h — Atmósfera de batalla (Fase 5): viñeta, sombras de nube, motas
#pragma once

#include "raylib.h"

void atmosInit();       // crea texturas (una vez, tras InitWindow)
void atmosShutdown();   // descarga texturas (antes de CloseWindow)
void atmosReset();      // reposiciona nubes/motas al iniciar batalla (initBattle)
void atmosUpdate(float dt); // dt con timeScale; no llamar en pausa
void atmosDrawWorld();  // dentro de beginBattleView (nubes + motas, sobre unidades)
void atmosDrawScreen(int w, int h); // viñeta de bordes, espacio de pantalla
