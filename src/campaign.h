// src/campaign.h — Mapa de campaña, provincias, turnos
#pragma once

#include "config.h"
#include <string>
#include <vector>
#include <utility>

// (:467, :470-473)
inline const char* terrainNames[4]={"Plain","Forest","Mountain","Coast"};
inline const char* factionNames[FACTION_COUNT]={"The Kingdom","Iron Pact","Stone Realm","Trade Republic","Neutral"};
inline const Color factionColors[FACTION_COUNT]={
    {60,120,220,255},{220,50,50,255},{180,90,30,255},{220,200,50,255},{120,120,100,255}
};

// (:455-461)
struct City {
    char     name[32];
    bool     built[BLD_COUNT];   // completed buildings
    int      constructing;       // -1 = nothing, else BuildingType
    int      constructTurns;     // turns remaining
    float    defBonus;           // from walls
};

// (:475-486)
struct Province {
    char       name[32];
    Vector2    center;          // on campaign map (pixel position)
    float      nx, ny;          // normalized position [0-1] for resize
    TerrainType terrain;
    FactionId  owner;
    bool       hasCity;
    City       city;
    std::vector<int> adjacent; // indices of adjacent provinces
    // Army stationed here (list of unit type indices + soldier counts)
    std::vector<std::pair<int,int>> army; // {typeIdx, soldierCount}
};

// Entrada de cola de reclutamiento (:558-562) — antes que CampaignState
struct RecruitEntry {
    int typeIdx;
    int turnsLeft;
    int originalTurns;  // 4.3: for progress bar
};

// (:567-594)
struct CampaignState {
    int              campaignId=0;     // which campaign map (0..MAX_CAMPAIGNS-1)
    int              turn=1;
    Resources        res;
    int              playerProvince=0;  // current province of player army
    std::vector<Province> provinces;
    // Player army: list of {typeIdx, soldierCount}
    std::vector<std::pair<int,int>> playerArmy;
    // Pending battles: province index where battle happens
    int              pendingBattleProvince=-1;
    bool             pendingBattleIsDefense=false;
    // City being viewed
    int              viewedCity=-1;
    // Recruitment queue per city (province index -> queue)
    std::vector<RecruitEntry> recruitQueue; // for viewed city
    // Available units (ready to deploy) - province index -> ready units
    std::vector<std::pair<int,int>> readyUnits;
    // 6.3: Fog of war — explored provinces
    bool             explored[32]={};   // true if province has been seen
    // Fase D+: facciones aliadas con el jugador (persistencia en Fase I)
    bool             allied[FACTION_COUNT]={};
    // 3.4: Stats for victory/defeat screen
    int              battlesWon=0;
    int              battlesLost=0;
    int              peakProvinces=0;
    // Army movement animation (campaign map)
    int              armyMoveFrom=-1;
    int              armyMoveTo=-1;
    float            armyMoveT=0.f;
};

// (:666-675)
struct PreBattleState {
    int  provinceIdx;
    bool isDefense;
    // Which of the player's units to include (booleans)
    std::vector<bool> include;
    int  deployedCount;
    // Enemy info
    bool fogOfWar;
    int  estimatedEnemyStrength;
};

extern CampaignState  g_campaign;  // (:596)
extern PreBattleState g_preBattle; // (:676)

// Fase D+: bando del jugador en el mapa (propio + aliados = mismo color;
// cada faccion enemiga conserva el suyo)
bool  factionIsPlayerSide(FactionId f);
Color factionDisplayColor(FactionId f);

// (:690, :692-693)
extern const float TRADE_RATES[5]; // gold, food, wood, stone, iron
extern int g_tradeTab;   // 0-4 for each resource
extern int g_tradeAmount; // amount to buy/sell

// (:773-774)
extern int g_deleteConfirmIdx;  // 4.4: garrison delete confirmation
extern int g_selectedProvince;  // 4.5: pulsing selected province

// Funciones (definiciones en campaign.cpp)
void generateCampaignMap(int campaignId);        // (:1039)
void updateProvinceCenters();                    // (:1214)
void newCampaign(int campaignId);                // (:1224)
void processTurn();                              // (:1248)
void drawGarrisonArmiesPanel(Province& prov, float gridY, float cellH, Vector2 mouse); // (:820)
void drawCampaignMovementArrows();               // (:849)
