// src/terrain.h — Obstáculos AABB por tipo de terreno
#pragma once
#include "config.h"
#include <vector>

// Extraído de initBattle (rts_game.cpp:1465-1485)
void generateTerrainObstacles(TerrainType terrain,int provinceIdx,std::vector<Rectangle>& out);
