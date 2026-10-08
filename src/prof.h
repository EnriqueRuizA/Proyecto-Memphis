// src/prof.h — Profiler ligero por secciones (Fase A rendimiento)
// Uso: profAdd(sec, ms) en cada seccion; profFrameEnd() al cerrar frame;
// overlay F10 (g_profOverlay) y log cada 120 frames a build/prof_log.txt.
#pragma once

enum ProfSec {
    PROF_FRAME=0,   // frame completo (BeginDrawing..EndDrawing)
    PROF_INPUT,     // batalla: entradas/camara/seleccion
    PROF_UPDATE,    // batalla: IA + unidades + separacion + fx + proyectiles
    PROF_SEP,       // separateAll() (subconjunto de UPDATE)
    PROF_DRAW,      // batalla: mundo + HUD
    PROF_TERRAIN,   // drawBattlefield() (subconjunto de DRAW)
    PROF_UNITS,     // drawAllUnits()+fx+debug (subconjunto de DRAW)
    PROF_MAP,       // mapa de campana completo
    PROF_COUNT
};

extern bool g_profOverlay;          // F10
extern int g_profSoldiers;          // soldados vivos (actualizado en batalla)
extern double g_profFrame[PROF_COUNT]; // acumulado del frame en curso (ms)
extern double g_profAvg[PROF_COUNT];   // media exponencial (ms)

void profAdd(int sec, double ms);
void profFrameBegin();
void profFrameEnd();       // EMA + log cada 120 frames
void profDrawOverlay();    // llama dentro de BeginDrawing/EndDrawing
