// src/terrain.cpp — Generación de obstáculos AABB por tipo de terreno
// Extraído literalmente de initBattle (rts_game.cpp:1465-1485).
#include "terrain.h"
#include <cstdlib>
#include <ctime>

void generateTerrainObstacles(TerrainType terrain,int provinceIdx,std::vector<Rectangle>& out){
    out.clear();
    if(terrain==TERRAIN_FOREST){
        // Clusters of trees as obstacle rects
        srand(provinceIdx*13337);
        for(int i=0;i<25;i++){
            float ox=(float)(rand()%BATTLE_W);
            float oy=(float)(rand()%BATTLE_H);
            float ow=40.f+(float)(rand()%60);
            float oh=40.f+(float)(rand()%60);
            out.push_back({ox,oy,ow,oh});
        }
    } else if(terrain==TERRAIN_MOUNTAIN){
        srand(provinceIdx*7777);
        for(int i=0;i<12;i++){
            float ox=(float)(rand()%BATTLE_W);
            float oy=(float)(rand()%BATTLE_H);
            out.push_back({ox,oy,80.f+rand()%120,80.f+rand()%120});
        }
    }
    srand((unsigned)time(nullptr));
}
