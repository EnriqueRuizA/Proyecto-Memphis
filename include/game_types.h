// MEDIEVAL CONQUEST — Tipos, constantes y enumeraciones compartidos
#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include "raylib.h"
#include <vector>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <ctime>

// ─── Constantes de pantalla (las que varían en runtime son extern en game_globals.h) ───
extern int SCREEN_W;
extern int SCREEN_H;
static const int HUD_H      = 96;
static const int TOPBAR_H   = 32;
static const int BATTLE_W   = 2560;
static const int BATTLE_H   = 1536;

// ─── Paleta global ───
static const Color C_BG        = {14,12,10,255};
static const Color C_GOLD      = {200,165,80,255};
static const Color C_PARCHMENT = {230,220,200,255};
static const Color C_SECONDARY = {140,130,110,255};
static const Color C_ALLY      = {80,160,220,255};
static const Color C_ENEMY_COL = {220,60,50,255};
static const Color C_TERRAIN   = {42,54,32,255};

// ─── Estados del juego ───
enum GameState {
    STATE_MAIN_MENU=0,
    STATE_CAMPAIGN_MAP,
    STATE_CITY_MANAGEMENT,
    STATE_RECRUITMENT,
    STATE_PRE_BATTLE,
    STATE_BATTLE,
    STATE_BATTLE_RESULT,
    STATE_UNIT_CODEX,
    STATE_SETTINGS,
    STATE_UNIT_EDITOR,
    STATE_QUICK_BATTLE_SETUP
};

enum SpriteBase { SPR_INFANTRY=0, SPR_CAVALRY, SPR_RANGED, SPR_BASE_COUNT };
static const char* spriteBaseNames[SPR_BASE_COUNT]={"Infantry","Cavalry","Ranged"};

struct UnitTypeDef {
    char    name[32];
    int     soldierCount;
    float   hpPerSoldier;
    int     armor, speed;
    int     meleeAttack, meleeDefense;
    float   meleeBaseDmg, meleeAPDmg, meleeInterval;
    int     range;
    float   missileBaseDmg, missileAPDmg, missileReload;
    int     recruitGold, recruitFood, recruitIron, recruitTurns;
    int     maintGold, maintFood;
    float   morale;
    unsigned char r,g,b;
    SpriteBase spriteBase;
    int     weaponHint;
    bool    isBuiltin;
    char    lore[256];
    int     buildingReqs;
};

struct Resources {
    float gold=500.f, food=200.f, wood=100.f, stone=50.f, iron=50.f;
};

enum BuildingType {
    BLD_MARKET=0, BLD_FARM, BLD_SAWMILL, BLD_QUARRY, BLD_SMITHY,
    BLD_BARRACKS, BLD_STABLE, BLD_RANGE, BLD_WORKSHOP,
    BLD_WALLS1, BLD_WALLS2, BLD_MAGETOWER, BLD_TEMPLE, BLD_PORT,
    BLD_COUNT
};

static const char* bldNames[BLD_COUNT]={
    "Market","Farm","Sawmill","Quarry","Smithy",
    "Barracks","Stable","Archery Range","Armoury Workshop",
    "Walls Lv.1","Walls Lv.2","Mage Tower","Temple","Port"
};
static const int bldGoldCost[BLD_COUNT]={200,120,150,200,300,200,350,250,400,300,600,500,250,400};
static const int bldWoodCost[BLD_COUNT]={0,0,0,0,0,60,100,80,120,0,0,0,0,100};
static const int bldStoneCost[BLD_COUNT]={0,0,0,0,0,0,0,0,0,150,300,200,0,0};
static const int bldIronCost[BLD_COUNT]={0,0,0,0,0,0,0,0,120,0,0,0,0,0};
static const int bldTurns[BLD_COUNT]={2,1,2,3,3,2,3,2,4,3,4,5,2,3};

static const int bldPrereq[BLD_COUNT]={-1,-1,1,-1,2,-1,5,5,4,-1,9,-1,-1,-1};

static const float bldGoldYield[BLD_COUNT]={80,0,0,0,0,0,0,0,0,0,0,0,0,120};
static const float bldFoodYield[BLD_COUNT]={0,60,0,0,0,0,0,0,0,0,0,0,0,0};
static const float bldWoodYield[BLD_COUNT]={0,0,50,0,0,0,0,0,0,0,0,0,0,0};
static const float bldStoneYield[BLD_COUNT]={0,0,0,40,0,0,0,0,0,0,0,0,0,0};
static const float bldIronYield[BLD_COUNT]={0,0,0,0,30,0,0,0,0,0,0,0,0,0};

struct City {
    char     name[32];
    bool     built[BLD_COUNT];
    int      constructing, constructTurns;
    float    defBonus;
};

enum TerrainType { TERRAIN_PLAIN=0, TERRAIN_FOREST, TERRAIN_MOUNTAIN, TERRAIN_COAST };
enum FactionId { FACTION_PLAYER=0, FACTION_AGGRESSIVE, FACTION_DEFENSIVE, FACTION_COMMERCIAL, FACTION_NEUTRAL, FACTION_COUNT };
static const char* terrainNames[4]={"Plain","Forest","Mountain","Coast"};
static const char* factionNames[FACTION_COUNT]={"The Kingdom","Iron Pact","Stone Realm","Trade Republic","Neutral"};
static const Color factionColors[FACTION_COUNT]={
    {60,120,220,255},{220,50,50,255},{180,90,30,255},{220,200,50,255},{120,120,100,255}
};

struct Province {
    char       name[32];
    Vector2    center;
    TerrainType terrain;
    FactionId  owner;
    bool       hasCity;
    City       city;
    std::vector<int> adjacent;
    std::vector<std::pair<int,int>> army;
};

enum SoldierState { SS_IDLE=0, SS_MOVING_SLOT, SS_MOVING_TARGET, SS_ATTACKING_MELEE, SS_ATTACKING_RANGED, SS_FLEEING };
enum UnitGroupState { UGS_IDLE=0, UGS_ADVANCING, UGS_ENGAGED, UGS_ROUTING };

struct Soldier {
    Vector2 pos, formationSlot;
    float hp, angle, angleTarget, meleeTimer, rangeTimer;
    bool alive, inMelee;
    int attackTargetSoldier, attackTargetUnit;
    SoldierState state;
    float chargeMoveTime;
    bool chargeReady;
};

struct BattleUnit {
    int typeIdx;
    bool isPlayer, selected;
    Vector2 anchorPos, orderTarget;
    int orderAttack;
    bool hasExplicitOrder;
    UnitGroupState groupState;
    float morale, moraleTimer, routeTimer;
    std::vector<Soldier> soldiers;
};

struct Projectile {
    Vector2 pos, vel;
    float dmg;
    bool alive, fromPlayer, isCrossbow;
};

struct DeadMarker {
    Vector2 pos;
    float alpha, timer;
    bool isCavalry;
};

struct RecruitEntry { int typeIdx; int turnsLeft; };

struct CampaignState {
    int turn=1;
    Resources res;
    int playerProvince=0;
    std::vector<Province> provinces;
    std::vector<std::pair<int,int>> playerArmy;
    int pendingBattleProvince=-1;
    bool pendingBattleIsDefense=false;
    int viewedCity=-1;
    std::vector<RecruitEntry> recruitQueue;
    std::vector<std::pair<int,int>> readyUnits;
};

struct BattleState {
    std::vector<BattleUnit> playerUnits, enemyUnits;
    std::vector<Projectile> projs;
    std::vector<DeadMarker> dead;
    Camera2D cam;
    float camZoom;
    std::vector<Rectangle> obstacles;
    TerrainType terrain;
    float timeScale;
    bool paused, dragging;
    Vector2 selStart;
    Rectangle selRect;
    int battleProvince;
    bool isDefense, battleOver, playerWon;
    float resultTimer, lootGold;
    char scenarioName[64];
};

struct BattleResult {
    bool playerWon;
    int playerLosses, enemyLosses;
    float lootGold;
    int provinceIdx;
    bool isDefense;
    std::vector<std::pair<int,int>> survivors;
};

struct PreBattleState {
    int provinceIdx;
    bool isDefense;
    std::vector<bool> include;
    int deployedCount;
    bool fogOfWar;
    int estimatedEnemyStrength;
};

struct QuickBattleSetup {
    std::vector<int> playerCounts, enemyCounts;
    int difficulty;
};

// Utilidades inline
static inline unsigned char clampU8(int v){ return (unsigned char)std::max(0,std::min(255,v)); }
static inline float vdist(Vector2 a,Vector2 b){ float dx=a.x-b.x,dy=a.y-b.y; return sqrtf(dx*dx+dy*dy); }
static inline Vector2 vnorm(Vector2 v){ float l=sqrtf(v.x*v.x+v.y*v.y); if(l<0.0001f) return {0.f,0.f}; return {v.x/l,v.y/l}; }
static inline float dirToAngle(Vector2 d){ return atan2f(-d.y,d.x)*RAD2DEG; }
static inline float lerpAngle(float c,float t,float s){ float d=t-c; while(d>180.f)d-=360.f; while(d<-180.f)d+=360.f; return c+d*s; }
static inline float frand(){ return (float)rand()/(float)RAND_MAX; }
static inline bool ptInRect(Vector2 p,Rectangle r){ return p.x>=r.x&&p.x<=r.x+r.width&&p.y>=r.y&&p.y<=r.y+r.height; }
static inline Vector2 v2add(Vector2 a,Vector2 b){return {a.x+b.x,a.y+b.y};}
static inline Vector2 v2sub(Vector2 a,Vector2 b){return {a.x-b.x,a.y-b.y};}
static inline Vector2 v2scale(Vector2 a,float s){return {a.x*s,a.y*s};}
static inline float v2dot(Vector2 a,Vector2 b){return a.x*b.x+a.y*b.y;}
static inline float v2len(Vector2 a){return sqrtf(a.x*a.x+a.y*a.y);}

#endif
