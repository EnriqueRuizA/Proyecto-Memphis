// ═══════════════════════════════════════════════════════════════════════════
//  MEDIEVAL CONQUEST — RTS de Campaña con Batallas en Tiempo Real
//  Single-file C++17 using Raylib
//  Compile: g++ -std=c++17 rts_game.cpp -lraylib -lm -o game
// ═══════════════════════════════════════════════════════════════════════════
#include "raylib.h"
#include <vector>
#include <deque>
#include <cmath>
#include <string>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <cassert>
#include <functional>
#include <random>

// ───────────────────────────────────────────────────────────────────────────
//  SAVE FILE CONSTANTS (must be before forward declarations)
// ───────────────────────────────────────────────────────────────────────────
static const char* CAMPAIGN_SAVE_FILE = "campaign_save.dat";
static const int   SAVE_VERSION       = 4;  // bumped for new fields

// ───────────────────────────────────────────────────────────────────────────
//  SCREEN / GLOBAL CONSTANTS
// ───────────────────────────────────────────────────────────────────────────
static int SCREEN_W = 1280;
static int SCREEN_H = 768;
static const int HUD_H      = 96;   // bottom HUD in battle
static const int TOPBAR_H   = 32;   // top bar in battle
static const int BATTLE_W   = 2560;
static const int BATTLE_H   = 1536;

// ───────────────────────────────────────────────────────────────────────────
//  UTILITY
// ───────────────────────────────────────────────────────────────────────────
static inline constexpr unsigned char clampU8(int v){ return (unsigned char)(v<0?0:(v>255?255:v)); }
static inline float vdist(Vector2 a,Vector2 b){
    float dx=a.x-b.x,dy=a.y-b.y; return sqrtf(dx*dx+dy*dy);
}
static inline Vector2 vnorm(Vector2 v){
    float l=sqrtf(v.x*v.x+v.y*v.y);
    if(l<0.0001f) return {0.f,0.f};
    return {v.x/l,v.y/l};
}
static inline float dirToAngle(Vector2 d){ return atan2f(-d.y,d.x)*RAD2DEG; }
static inline float lerpAngle(float c,float t,float s){
    float d=t-c;
    while(d>180.f)d-=360.f; while(d<-180.f)d+=360.f;
    return c+d*s;
}
static inline float frand(){ return (float)rand()/(float)RAND_MAX; }
static inline bool ptInRect(Vector2 p,Rectangle r){
    return p.x>=r.x&&p.x<=r.x+r.width&&p.y>=r.y&&p.y<=r.y+r.height;
}
static inline Vector2 v2add(Vector2 a,Vector2 b){return {a.x+b.x,a.y+b.y};}
static inline Vector2 v2sub(Vector2 a,Vector2 b){return {a.x-b.x,a.y-b.y};}
static inline Vector2 v2scale(Vector2 a,float s){return {a.x*s,a.y*s};}
static inline float v2dot(Vector2 a,Vector2 b){return a.x*b.x+a.y*b.y;}
static inline float v2len(Vector2 a){return sqrtf(a.x*a.x+a.y*a.y);}

// ───────────────────────────────────────────────────────────────────────────
//  GLOBAL PALETTES
// ───────────────────────────────────────────────────────────────────────────
static const Color C_BG        = {14,12,10,255};
static const Color C_GOLD      = {200,165,80,255};
static const Color C_COPPER    = {160,80,40,255};
static const Color C_PARCHMENT = {230,220,200,255};
static const Color C_SECONDARY = {140,130,110,255};
static const Color C_BLOOD     = {160,30,20,255};
static const Color C_ALLY      = {80,160,220,255};
static const Color C_ENEMY_COL = {220,60,50,255};
static const Color C_TERRAIN   = {42,54,32,255};

// ───────────────────────────────────────────────────────────────────────────
//  GAME STATES
// ───────────────────────────────────────────────────────────────────────────
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
    STATE_UNIT_EDITOR,    // sandbox only from main menu
    STATE_QUICK_BATTLE_SETUP,
    STATE_VICTORY,
    STATE_DEFEAT,
    STATE_MARKETPLACE      // 6.5: resource trading
};

// ───────────────────────────────────────────────────────────────────────────
//  UNIT TYPE DEFINITIONS
// ───────────────────────────────────────────────────────────────────────────
enum SpriteBase { SPR_INFANTRY=0, SPR_CAVALRY, SPR_RANGED, SPR_BASE_COUNT };
static const char* spriteBaseNames[SPR_BASE_COUNT]={"Infantry","Cavalry","Ranged"};

struct UnitTypeDef {
    char    name[32];
    int     soldierCount;   // total soldiers per group
    float   hpPerSoldier;
    int     armor;          // 0-40
    int     speed;          // px/s
    int     meleeAttack;
    int     meleeDefense;
    float   meleeBaseDmg;
    float   meleeAPDmg;
    float   meleeInterval;  // seconds between attacks
    int     range;          // 0 = melee only
    float   missileBaseDmg;
    float   missileAPDmg;
    float   missileReload;
    int     recruitGold;
    int     recruitFood;
    int     recruitIron;
    int     recruitTurns;
    int     maintGold;
    int     maintFood;
    // Morale base (0-100)
    float   morale;
    // Display
    unsigned char r,g,b;
    SpriteBase spriteBase;
    int     weaponHint;    // 0=spear,1=axe,2=poleaxe,3=hammer
    bool    isBuiltin;
    // Lore
    char    lore[256];
    // Building requirements (bitmask index into BuildingType enum)
    // encoded as bit flags for simplicity
    int     buildingReqs;  // bit field: 0=Barracks,1=Stable,2=Range,3=Smithy,4=Workshop
};

static std::vector<UnitTypeDef> g_unitTypes;
static std::vector<Texture2D>   g_playerTextures;
static std::vector<Texture2D>   g_enemyTextures;

// pixel helpers for 32x32 sprites
static void px32(Image& img,int x,int y,Color c){
    if(x<0||y<0||x>=img.width||y>=img.height)return;
    ImageDrawPixel(&img,x,y,c);
}
static void fillRect32(Image& img,int x,int y,int w,int h,Color c){
    for(int yy=y;yy<y+h;yy++)for(int xx=x;xx<x+w;xx++)px32(img,xx,yy,c);
}
static void circle32(Image& img,int cx,int cy,int r,Color c,bool fill=true){
    for(int dy=-r;dy<=r;dy++){
        for(int dx=-r;dx<=r;dx++){
            int d2=dx*dx+dy*dy;
            if(fill?(d2<=r*r):((d2>=(r-1)*(r-1))&&(d2<=r*r)))
                px32(img,cx+dx,cy+dy,c);
        }
    }
}

// Build infantry 32x32 sprite (geometric, semi-realistic)
static Image makeInfantrySprite(unsigned char r,unsigned char g,unsigned char b,int weaponHint){
    Image img=GenImageColor(32,32,BLANK);
    Color body={r,g,b,255};
    Color dark={clampU8(r/2),clampU8(g/2),clampU8(b/2),255};
    Color hi={clampU8(r+60),clampU8(g+60),clampU8(b+60),255};
    Color skin={210,165,110,255};
    Color metal={180,180,190,255};
    Color shad={0,0,0,70};

    // Shadow
    circle32(img,16,28,7,shad,true);

    // Legs
    fillRect32(img,11,20,4,8,dark);
    fillRect32(img,17,20,4,8,dark);
    // Body (torso)
    circle32(img,16,16,7,body,true);
    // Highlight
    circle32(img,13,13,3,hi,true);
    // Head
    circle32(img,16,9,5,skin,true);
    // Helmet
    fillRect32(img,11,5,10,5,metal);
    px32(img,11,6,dark); px32(img,20,6,dark);

    // Weapon
    if(weaponHint==0){ // spear
        for(int y2=0;y2<=12;y2++) px32(img,22,y2,metal);
        fillRect32(img,20,0,4,3,{220,220,100,255}); // tip
    } else if(weaponHint==1){ // axe
        for(int y2=10;y2<=20;y2++) px32(img,22,y2,{140,100,50,255});
        fillRect32(img,20,7,6,5,metal);
        px32(img,20,7,{180,100,40,255});px32(img,25,7,{180,100,40,255});
    } else if(weaponHint==2){ // poleaxe
        for(int y2=0;y2<=18;y2++) px32(img,22,y2,metal);
        fillRect32(img,19,0,6,3,metal);
        fillRect32(img,19,3,4,3,{220,200,60,255});
    } else { // hammer
        for(int y2=10;y2<=20;y2++) px32(img,22,y2,{140,100,50,255});
        fillRect32(img,18,7,8,5,{160,160,180,255});
    }
    // Direction indicator (small triangle at top = forward)
    px32(img,16,0,C_GOLD); px32(img,15,1,C_GOLD); px32(img,17,1,C_GOLD);
    return img;
}

static Image makeCavalrySprite(unsigned char r,unsigned char g,unsigned char b){
    Image img=GenImageColor(32,32,BLANK);
    Color body={r,g,b,255};
    Color dark={clampU8(r/2),clampU8(g/2),clampU8(b/2),255};
    Color hi={clampU8(r+50),clampU8(g+50),clampU8(b+50),255};
    Color horse={110,75,35,255};
    Color hdark={70,45,20,255};
    Color metal={190,190,200,255};
    Color skin={210,165,110,255};
    Color shad={0,0,0,70};

    // Shadow
    fillRect32(img,5,28,22,4,shad);

    // Horse body (oval)
    for(int dy=-8;dy<=8;dy++)
        for(int dx=-12;dx<=12;dx++){
            float e=(float)(dx*dx)/144.f+(float)(dy*dy)/64.f;
            if(e<=1.f) px32(img,16+dx,20+dy,horse);
        }
    // Horse legs
    fillRect32(img,6,26,3,6,hdark);
    fillRect32(img,11,27,3,5,hdark);
    fillRect32(img,18,27,3,5,hdark);
    fillRect32(img,23,26,3,6,hdark);
    // Horse neck + head
    fillRect32(img,22,13,5,8,horse);
    circle32(img,25,10,4,horse,true);
    // Rider body
    circle32(img,14,13,6,body,true);
    px32(img,11,11,hi); px32(img,12,10,hi);
    // Rider head
    circle32(img,13,7,4,skin,true);
    fillRect32(img,9,4,8,4,metal); // helmet
    // Lance
    for(int y2=0;y2<=14;y2++) px32(img,26,y2,metal);
    px32(img,26,0,{220,220,80,255});
    // Direction indicator
    px32(img,16,0,C_GOLD); px32(img,15,1,C_GOLD); px32(img,17,1,C_GOLD);
    (void)dark;
    return img;
}

static Image makeRangedSprite(unsigned char r,unsigned char g,unsigned char b,bool crossbow){
    Image img=GenImageColor(32,32,BLANK);
    Color body={r,g,b,255};
    Color dark={clampU8(r/2),clampU8(g/2),clampU8(b/2),255};
    Color hi={clampU8(r+60),clampU8(g+60),clampU8(b+60),255};
    Color skin={210,165,110,255};
    Color wood={160,120,60,255};
    Color shad={0,0,0,70};

    circle32(img,16,28,6,shad,true);
    fillRect32(img,11,20,4,8,dark);
    fillRect32(img,17,20,4,8,dark);
    circle32(img,16,16,7,body,true);
    circle32(img,13,13,3,hi,true);
    circle32(img,16,9,5,skin,true);
    fillRect32(img,11,5,10,4,{80,100,70,255}); // hood/cap

    if(!crossbow){ // bow
        // bow arc
        for(int i=0;i<=6;i++){
            float a=(float)i/6.f*3.14159f;
            int bx=22+(int)(5*cosf(a));
            int by=8+(int)(8*sinf(a));
            px32(img,bx,by,wood);
        }
        // string
        px32(img,22,8,{200,200,200,255});
        px32(img,22,16,{200,200,200,255});
        // arrow
        for(int y2=9;y2<=14;y2++) px32(img,23,y2,{180,140,60,255});
    } else { // crossbow
        fillRect32(img,19,13,9,3,wood); // stock
        fillRect32(img,18,10,11,2,{100,80,30,255}); // prod
        fillRect32(img,22,10,2,7,wood); // tiller
        px32(img,22,13,{160,160,160,255}); // trigger
        // bolt
        px32(img,28,11,{180,140,60,255});
        px32(img,29,11,{220,180,80,255});
    }
    px32(img,16,0,C_GOLD); px32(img,15,1,C_GOLD); px32(img,17,1,C_GOLD);
    (void)hi; (void)dark;
    return img;
}

static Texture2D imageToTex(Image img){
    Texture2D tex=LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(tex,TEXTURE_FILTER_BILINEAR);
    return tex;
}

static void rebuildTexture(int idx){
    const UnitTypeDef& td=g_unitTypes[idx];
    auto makeImg=[&](unsigned char r,unsigned char g,unsigned char b)->Image{
        switch(td.spriteBase){
            case SPR_CAVALRY: return makeCavalrySprite(r,g,b);
            case SPR_RANGED:  return makeRangedSprite(r,g,b,(td.range>0&&td.missileReload>2.5f));
            default:          return makeInfantrySprite(r,g,b,td.weaponHint);
        }
    };
    auto setOrPush=[&](std::vector<Texture2D>& vec,Texture2D tex,int i){
        if(i<(int)vec.size()){UnloadTexture(vec[i]);vec[i]=tex;}
        else vec.push_back(tex);
    };
    setOrPush(g_playerTextures,imageToTex(makeImg(td.r,td.g,td.b)),idx);
    unsigned char er=clampU8((int)(td.r*0.3f+160));
    unsigned char eg=clampU8((int)(td.g*0.2f+20));
    unsigned char eb=clampU8((int)(td.b*0.2f+10));
    setOrPush(g_enemyTextures,imageToTex(makeImg(er,eg,eb)),idx);
}

static void initBuiltinTypes(){
    g_unitTypes.clear();
    // Helper lambda
    auto add=[&](const char* name,int soldiers,float hp,int armor,int spd,
                 int matk,int mdef,float mbase,float map2,float mint,
                 int rng,float msbase,float msap,float mrel,
                 int rGold,int rFood,int rIron,int rTurns,int mGold,int mFood,
                 float morale,unsigned char r,unsigned char g,unsigned char b,
                 SpriteBase spr,int wh,int bReqs,const char* lore){
        UnitTypeDef t{};
        strncpy(t.name,name,sizeof(t.name)-1); t.name[sizeof(t.name)-1]='\0';
        t.soldierCount=soldiers; t.hpPerSoldier=hp; t.armor=armor; t.speed=spd;
        t.meleeAttack=matk; t.meleeDefense=mdef;
        t.meleeBaseDmg=mbase; t.meleeAPDmg=map2; t.meleeInterval=mint;
        t.range=rng; t.missileBaseDmg=msbase; t.missileAPDmg=msap; t.missileReload=mrel;
        t.recruitGold=rGold; t.recruitFood=rFood; t.recruitIron=rIron;
        t.recruitTurns=rTurns; t.maintGold=mGold; t.maintFood=mFood;
        t.morale=morale;
        t.r=r; t.g=g; t.b=b; t.spriteBase=spr; t.weaponHint=wh;
        t.isBuiltin=true; t.buildingReqs=bReqs;
        strncpy(t.lore,lore,sizeof(t.lore)-1); t.lore[sizeof(t.lore)-1]='\0';
        g_unitTypes.push_back(t);
    };

    // Bit flags for building reqs: 0=Barracks,1=Stable,2=Range,3=Smithy,4=Workshop
    //          name              sol hp  arm spd  matk mdef mbase map  mint  rng msbase msap mrel  rGold rFood rIron rT mG  mF  mor   r    g    b       spr          wh  breqs  lore
    add("Spear Levy",             60, 12, 0,  80,  18,  12,  8.f, 1.5f,1.8f, 0,  0.f,  0.f, 0.f,  180,  30,   0,   1,  20, 20, 80.f, 200,180,140, SPR_INFANTRY,  0,  1,   "Peasant levy armed with spears. Weak but cheap and numerous.");
    add("Axe Militia",            60, 14, 5,  75,  24,  16, 12.f, 3.f, 1.6f, 0,  0.f,  0.f, 0.f,  240,  40,   0,   1,  28, 25, 78.f, 160,110, 60, SPR_INFANTRY,  1,  1,   "Free citizens wielding axes. Better offense than spear levy.");
    add("Poleaxe Retinue",        40, 22, 14, 70,  36,  24, 18.f, 7.f, 1.4f, 0,  0.f,  0.f, 0.f,  400,  60,  60,   2,  45, 20, 85.f, 140,150,160, SPR_INFANTRY,  2,  9,   "Professional soldiers wielding poleaxes. Heavy armor, reliable.");
    add("Dismounted Knights",     30, 38, 22, 60,  44,  30, 22.f,13.f, 1.6f, 0,  0.f,  0.f, 0.f,  700, 100, 100,   3,  80, 15, 90.f, 200,210,220, SPR_INFANTRY,  3,  17,  "Elite knights fighting on foot. Exceptional armor and morale.");
    add("Bow Levy",               60, 11, 0,  85,  16,  10,  7.f, 1.f, 2.0f,260, 10.f, 1.5f,2.5f, 200,  30,   0,   1,  22, 20, 75.f, 190,170,100, SPR_RANGED,    0,  4,   "Peasant archers. Low rate of fire but plentiful and cheap.");
    add("Crossbow Retinue",       40, 18, 8,  70,  22,  18,  9.f, 2.f, 2.2f,300, 16.f, 7.f, 4.0f, 420,  60,  60,   2,  50, 20, 82.f,  80,130, 80, SPR_RANGED,    0,  12,  "Trained crossbowmen. Slow reload but punishing armor penetration.");
    add("Mounted Sergeants",      40, 20, 8, 160,  28,  20, 15.f, 4.f, 1.4f, 0,  0.f,  0.f, 0.f,  520,  80,  40,   2,  60, 30, 83.f, 140, 90, 60, SPR_CAVALRY,   0,  2,   "Light cavalry. Swift flankers and pursuit specialists.");
    add("Knights",                24, 42, 26,140,  50,  34, 26.f,18.f, 1.5f, 0,  0.f,  0.f, 0.f,  960, 120, 120,   4, 110, 25, 92.f, 220,220,200, SPR_CAVALRY,   0,  18,  "Heavy armored knights. Devastating charge, supreme melee prowess.");

    g_playerTextures.clear();
    g_enemyTextures.clear();
    for(int i=0;i<(int)g_unitTypes.size();i++) rebuildTexture(i);
}

static int unitTypeCount(){ return (int)g_unitTypes.size(); }

// ───────────────────────────────────────────────────────────────────────────
//  RESOURCES
// ───────────────────────────────────────────────────────────────────────────
struct Resources {
    float gold=500.f, food=200.f, wood=100.f, stone=50.f, iron=50.f;
};

// ───────────────────────────────────────────────────────────────────────────
//  BUILDINGS
// ───────────────────────────────────────────────────────────────────────────
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
static const int bldPrereq[BLD_COUNT]={-1,-1,1,-1,2,-1,5,5,4,-1,9,-1,-1,-1}; // Smithy req Sawmill, Stable/Range req Barracks, Workshop req Smithy, Walls2 req Walls1

// Gold/food/wood/stone/iron per turn per building
static const float bldGoldYield[BLD_COUNT]={80,0,0,0,0,0,0,0,0,0,0,0,0,120};
static const float bldFoodYield[BLD_COUNT]={0,60,0,0,0,0,0,0,0,0,0,0,0,0};
static const float bldWoodYield[BLD_COUNT]={0,0,50,0,0,0,0,0,0,0,0,0,0,0};
static const float bldStoneYield[BLD_COUNT]={0,0,0,40,0,0,0,0,0,0,0,0,0,0};
static const float bldIronYield[BLD_COUNT]={0,0,0,0,30,0,0,0,0,0,0,0,0,0};

struct City {
    char     name[32];
    bool     built[BLD_COUNT];   // completed buildings
    int      constructing;       // -1 = nothing, else BuildingType
    int      constructTurns;     // turns remaining
    float    defBonus;           // from walls
};

// ───────────────────────────────────────────────────────────────────────────
//  PROVINCE / CAMPAIGN MAP
// ───────────────────────────────────────────────────────────────────────────
enum TerrainType { TERRAIN_PLAIN=0, TERRAIN_FOREST, TERRAIN_MOUNTAIN, TERRAIN_COAST };
static const char* terrainNames[4]={"Plain","Forest","Mountain","Coast"};

enum FactionId { FACTION_PLAYER=0, FACTION_AGGRESSIVE, FACTION_DEFENSIVE, FACTION_COMMERCIAL, FACTION_NEUTRAL, FACTION_COUNT };
static const char* factionNames[FACTION_COUNT]={"The Kingdom","Iron Pact","Stone Realm","Trade Republic","Neutral"};
static const Color factionColors[FACTION_COUNT]={
    {60,120,220,255},{220,50,50,255},{180,90,30,255},{220,200,50,255},{120,120,100,255}
};

struct Province {
    char       name[32];
    Vector2    center;          // on campaign map (0-1 normalized)
    TerrainType terrain;
    FactionId  owner;
    bool       hasCity;
    City       city;
    std::vector<int> adjacent; // indices of adjacent provinces
    // Army stationed here (list of unit type indices + soldier counts)
    std::vector<std::pair<int,int>> army; // {typeIdx, soldierCount}
};

// ───────────────────────────────────────────────────────────────────────────
//  SOLDIER (individual entity in battle)
// ───────────────────────────────────────────────────────────────────────────
enum SoldierState { SS_IDLE=0, SS_MOVING_SLOT, SS_MOVING_TARGET, SS_ATTACKING_MELEE, SS_ATTACKING_RANGED, SS_FLEEING };

struct Soldier {
    Vector2      pos;
    Vector2      formationSlot;
    float        hp;
    float        angle, angleTarget;
    float        meleeTimer, rangeTimer;
    bool         alive;
    bool         inMelee;
    int          attackTargetSoldier; // index in enemy BattleUnit's soldiers array
    int          attackTargetUnit;    // which enemy BattleUnit
    SoldierState state;
    float        chargeMoveTime;     // accumulated time moving at full speed
    bool         chargeReady;
    float        targetRefreshTimer; // 2.3: cache nearest enemy, refresh every 0.3s
    int          kills;              // 6.2: for veterancy
};

// ───────────────────────────────────────────────────────────────────────────
//  BATTLE UNIT (group of soldiers)
// ───────────────────────────────────────────────────────────────────────────
enum UnitGroupState { UGS_IDLE=0, UGS_ADVANCING, UGS_ENGAGED, UGS_ROUTING };

struct BattleUnit {
    int       typeIdx;
    bool      isPlayer;
    bool      selected;
    Vector2   anchorPos;
    Vector2   orderTarget;
    int       orderAttack;     // index into enemy BattleUnit list (-1=none)
    bool      hasExplicitOrder;
    UnitGroupState groupState;
    float     morale;
    float     moraleTimer;     // timer since last damage for recovery
    float     routeTimer;      // seconds left in routing
    std::vector<Soldier> soldiers;
    int       veterancy;       // 6.2: 0=Recruit,1=Seasoned,2=Veteran,3=Elite
    float     aiReactTimer;    // 3.2: AI recalculates orders every 1.5s
    int       orderType;       // 6.6: 0=attack,1=move,2=hold,3=flank
    float     formationFacing; // 6.6: rotation angle for formation
    float     lastFormationFacing; // v7.2: track last facing to detect changes
};

// ───────────────────────────────────────────────────────────────────────────
//  PROJECTILE
// ───────────────────────────────────────────────────────────────────────────
struct Projectile {
    Vector2 pos, vel;
    float   dmg;
    bool    alive, fromPlayer;
    bool    isCrossbow; // affects color/speed
};

// ───────────────────────────────────────────────────────────────────────────
//  DEAD MARKER
// ───────────────────────────────────────────────────────────────────────────
struct DeadMarker {
    Vector2 pos;
    float   alpha;
    float   timer;
    bool    isCavalry;
};

// ───────────────────────────────────────────────────────────────────────────
//  RECRUITMENT QUEUE ENTRY
// ───────────────────────────────────────────────────────────────────────────
struct RecruitEntry {
    int typeIdx;
    int turnsLeft;
    int originalTurns;  // 4.3: for progress bar
};

// ───────────────────────────────────────────────────────────────────────────
//  GLOBAL CAMPAIGN STATE
// ───────────────────────────────────────────────────────────────────────────
struct CampaignState {
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
    // 3.4: Stats for victory/defeat screen
    int              battlesWon=0;
    int              battlesLost=0;
    int              peakProvinces=0;
};

static CampaignState g_campaign;

// ───────────────────────────────────────────────────────────────────────────
//  BATTLE STATE (used during STATE_BATTLE)
// ───────────────────────────────────────────────────────────────────────────
struct BattleState {
    std::vector<BattleUnit>  playerUnits;
    std::vector<BattleUnit>  enemyUnits;
    std::vector<Projectile>  projs;
    std::vector<DeadMarker>  dead;
    // Camera
    Camera2D  cam;
    float     camZoom;
    // Battlefield terrain obstacles (AABB)
    std::vector<Rectangle>   obstacles;
    // Terrain type (affects generated obstacles)
    TerrainType terrain;
    // Time control
    float     timeScale;     // 1.0 or 2.0
    bool      paused;
    // Selection
    bool      dragging;
    Vector2   selStart;
    Rectangle selRect;
    // Province index being fought over
    int       battleProvince;
    bool      isDefense;
    // Result
    bool      battleOver;
    bool      playerWon;
    float     resultTimer;
    // Loot
    float     lootGold;
    // Scenario name
    char      scenarioName[64];
};

static BattleState g_battle;

// ───────────────────────────────────────────────────────────────────────────
//  BATTLE RESULT DATA (passed to STATE_BATTLE_RESULT)
// ───────────────────────────────────────────────────────────────────────────
struct BattleResult {
    bool  playerWon;
    int   playerLosses;
    int   enemyLosses;
    float lootGold;
    int   provinceIdx;
    bool  isDefense;
    bool  lootApplied;   // 0.2: was static local, now per-result field
    // Survivor counts per unit type
    std::vector<std::pair<int,int>> survivors; // {typeIdx, survivors}
};
static BattleResult g_lastResult;

// ───────────────────────────────────────────────────────────────────────────
//  PRE-BATTLE STATE
// ───────────────────────────────────────────────────────────────────────────
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
static PreBattleState g_preBattle;

// ───────────────────────────────────────────────────────────────────────────
//  GLOBAL GAME STATE
// ───────────────────────────────────────────────────────────────────────────
static GameState g_state = STATE_MAIN_MENU;
static bool      g_quitRequested = false;
static bool      g_hasSave = false;
static float     g_menuTime = 0.f;

// ───────────────────────────────────────────────────────────────────────────
//  RESOURCE TRADING SYSTEM (6.5)
// ───────────────────────────────────────────────────────────────────────────
// Exchange rates: 1 unit of resource = X gold
static const float TRADE_RATES[5]={1.f, 1.2f, 1.3f, 1.5f, 0.8f}; // gold, food, wood, stone, iron
enum TradeResource { TRADE_GOLD=0, TRADE_FOOD=1, TRADE_WOOD=2, TRADE_STONE=3, TRADE_IRON=4 };
static int g_tradeTab=0; // 0-4 for each resource
static int g_tradeAmount=10; // amount to buy/sell

// ───────────────────────────────────────────────────────────────────────────
//  SETTINGS (3.1)
// ───────────────────────────────────────────────────────────────────────────
static const char* SETTINGS_FILE = "settings.ini";
struct GameSettings {
    float masterVolume = 1.f;
    float musicVolume  = 0.7f;
    int   difficulty   = 1;   // 0=EASY,1=NORMAL,2=HARD
    bool  showFPS      = false;
    int   language     = 0;   // 0=EN,1=ES
};
static GameSettings g_settings;

static void saveSettings(){
    FILE* f=fopen(SETTINGS_FILE,"w");
    if(!f) return;
    fprintf(f,"masterVolume %.3f\n",g_settings.masterVolume);
    fprintf(f,"musicVolume %.3f\n",g_settings.musicVolume);
    fprintf(f,"difficulty %d\n",g_settings.difficulty);
    fprintf(f,"showFPS %d\n",(int)g_settings.showFPS);
    fprintf(f,"language %d\n",g_settings.language);
    fclose(f);
}
static void loadSettings(){
    FILE* f=fopen(SETTINGS_FILE,"r");
    if(!f) return;
    char key[64];
    while(fscanf(f,"%63s",key)==1){
        if(strcmp(key,"masterVolume")==0) fscanf(f,"%f",&g_settings.masterVolume);
        else if(strcmp(key,"musicVolume")==0) fscanf(f,"%f",&g_settings.musicVolume);
        else if(strcmp(key,"difficulty")==0) fscanf(f,"%d",&g_settings.difficulty);
        else if(strcmp(key,"showFPS")==0){ int v=0; fscanf(f,"%d",&v); g_settings.showFPS=(bool)v; }
        else if(strcmp(key,"language")==0) fscanf(f,"%d",&g_settings.language);
    }
    fclose(f);
}

// ───────────────────────────────────────────────────────────────────────────
//  GLOBAL RNG (1.2)
// ───────────────────────────────────────────────────────────────────────────
static std::mt19937 g_rng(std::random_device{}());
static inline float frandMT(){ return std::uniform_real_distribution<float>(0.f,1.f)(g_rng); }
static inline int randIntMT(int lo,int hi){ return std::uniform_int_distribution<int>(lo,hi)(g_rng); }

// ───────────────────────────────────────────────────────────────────────────
//  BATTLE LOG (6.4)
// ───────────────────────────────────────────────────────────────────────────
static std::deque<std::string> g_battleLog;
static void battleLogAdd(const std::string& s){
    g_battleLog.push_front(s);
    if((int)g_battleLog.size()>50) g_battleLog.pop_back();
}

// ───────────────────────────────────────────────────────────────────────────
//  STATE FADE SYSTEM (4.2)
// ───────────────────────────────────────────────────────────────────────────
static float     g_fadeAlpha     = 0.f;
static GameState g_pendingState  = STATE_MAIN_MENU;
static bool      g_fadingOut     = false;

// ───────────────────────────────────────────────────────────────────────────
//  UI STATE GLOBALS (4.4, 4.5)
// ───────────────────────────────────────────────────────────────────────────
static int g_deleteConfirmIdx  = -1;  // 4.4: garrison delete confirmation
static int g_selectedProvince  = -1;  // 4.5: pulsing selected province

// ───────────────────────────────────────────────────────────────────────────
//  VALIDATING INDEX HELPER (5.4)
// ───────────────────────────────────────────────────────────────────────────
template<typename T>
static inline bool validIdx(int i, const std::vector<T>& v){ return i>=0 && i<(int)v.size(); }

// For quick battle (no campaign)
static bool g_quickBattle = false;
struct QuickBattleSetup {
    std::vector<int> playerCounts;
    std::vector<int> enemyCounts;
    int difficulty; // 0=easy,1=normal,2=hard
};
static QuickBattleSetup g_quickSetup;

// ───────────────────────────────────────────────────────────────────────────
//  UNIT EDITOR STATE (sandbox)
// ───────────────────────────────────────────────────────────────────────────
static int      g_editTypeIdx = 0;
static bool     g_editorDropdownOpen = false;
static bool     g_editorPreviewDirty = true;
static Texture2D g_editorPreviewTex = {0};
static float    g_editorPreviewAngle = 0.f;

// ───────────────────────────────────────────────────────────────────────────
//  FORWARD DECLARATIONS
// ───────────────────────────────────────────────────────────────────────────
static void saveGame();
static bool loadGame();
static void drawGarrisonArmiesPanel(Province& prov, float gridY, float cellH, Vector2 mouse);
static void drawCampaignMovementArrows();
static GameState updateDrawVictory(Vector2 mouse);
static GameState updateDrawDefeat(Vector2 mouse);

// ───────────────────────────────────────────────────────────────────────────
//  GARRISON ARMIES PANEL (0.1: extracted from orphan code)
// ───────────────────────────────────────────────────────────────────────────
static void drawGarrisonArmiesPanel(Province& prov, float gridY, float cellH, Vector2 mouse){
    // Draw a panel showing garrisoned armies for the given province
    // Currently shown inline in city management as part of the building grid;
    // this extracted function can be called for additional overlay
    float panelY=gridY+3*cellH+8;
    if(panelY+60>SCREEN_H-44) return;
    DrawRectangle(8,(int)panelY,(int)(SCREEN_W-16),54,{10,16,10,180});
    DrawRectangleLinesEx({8,panelY,(float)(SCREEN_W-16),54},1,{60,90,50,180});
    DrawText("Garrison:",(int)18,(int)(panelY+6),13,C_GOLD);
    if(prov.army.empty()){
        DrawText("(no garrison)",(int)100,(int)(panelY+8),12,C_SECONDARY);
    } else {
        int ax=100;
        for(int k=0;k<(int)prov.army.size()&&k<8;k++){
            auto [ti,cnt]=prov.army[k];
            if(ti>=unitTypeCount()) continue;
            const UnitTypeDef& td=g_unitTypes[ti];
            DrawRectangle(ax,(int)(panelY+6),14,14,{td.r,td.g,td.b,220});
            DrawText(TextFormat("%s x%d",td.name,cnt),ax+18,(int)(panelY+8),11,C_PARCHMENT);
            ax+=120+MeasureText(td.name,11);
        }
    }
    (void)mouse;
    (void)cellH;
}

// ───────────────────────────────────────────────────────────────────────────
//  CAMPAIGN MOVEMENT ARROWS (0.1: extracted function)
// ───────────────────────────────────────────────────────────────────────────
static void drawCampaignMovementArrows(){
    // Draw arrows from player province to adjacent reachable provinces
    if(g_campaign.playerProvince<0||g_campaign.playerProvince>=(int)g_campaign.provinces.size()) return;
    Province& pp=g_campaign.provinces[g_campaign.playerProvince];
    for(int adj:pp.adjacent){
        if(adj<0||adj>=(int)g_campaign.provinces.size()) continue;
        Province& ap=g_campaign.provinces[adj];
        Vector2 dir=vnorm(v2sub(ap.center,pp.center));
        Vector2 mid={pp.center.x+dir.x*40.f,pp.center.y+dir.y*40.f};
        // Small arrow pointing toward adjacent
        Color arrowCol=(ap.owner==FACTION_PLAYER)?C_ALLY:C_ENEMY_COL;
        DrawLineEx(mid,v2add(mid,v2scale(dir,24.f)),2.f,{arrowCol.r,arrowCol.g,arrowCol.b,120});
        // Arrowhead
        Vector2 tip=v2add(mid,v2scale(dir,24.f));
        Vector2 perp={-dir.y,dir.x};
        DrawTriangle(tip,
            v2sub(v2sub(tip,v2scale(dir,8.f)),v2scale(perp,5.f)),
            v2add(v2sub(tip,v2scale(dir,8.f)),v2scale(perp,5.f)),
            {arrowCol.r,arrowCol.g,arrowCol.b,100});
    }
}


static bool drawButton(Rectangle r,const char* lbl,Vector2 m,
                       Color cn={40,55,40,255},Color ch={70,110,60,255}){
    bool hv=ptInRect(m,r);
    DrawRectangleRec(r,hv?ch:cn);
    DrawRectangleLinesEx(r,2,hv?C_GOLD:Color{80,110,60,255});
    int fs=18, tw=MeasureText(lbl,fs);
    DrawText(lbl,(int)(r.x+r.width/2-tw/2),(int)(r.y+r.height/2-fs/2),fs,hv?C_PARCHMENT:C_SECONDARY);
    return hv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

static bool drawSmBtn(Rectangle r,const char* lbl,Vector2 m,
                      Color cn={30,40,30,255},Color ch={55,90,45,255}){
    bool hv=ptInRect(m,r);
    DrawRectangleRec(r,hv?ch:cn);
    DrawRectangleLinesEx(r,1,hv?C_GOLD:Color{60,90,50,255});
    int fs=13, tw=MeasureText(lbl,fs);
    DrawText(lbl,(int)(r.x+r.width/2-tw/2),(int)(r.y+r.height/2-fs/2),fs,hv?Color{255,255,255,255}:C_SECONDARY);
    return hv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

static int drawIntSlider(Rectangle r,int val,int mn,int mx,
                         const char* label,Vector2 mouse,Color fill={80,160,80,200}){
    if(mx<=mn) return val; // 1.5: guard division by zero
    if(label&&label[0]) DrawText(label,(int)r.x,(int)(r.y-15),12,C_SECONDARY);
    DrawRectangleRec(r,{18,18,18,255});
    DrawRectangleLinesEx(r,1,{55,55,55,255});
    float t=(float)(val-mn)/(float)(mx-mn);
    DrawRectangle((int)r.x,(int)r.y,(int)(t*r.width),(int)r.height,fill);
    float kx=r.x+t*r.width;
    DrawRectangle((int)(kx-4),(int)(r.y-2),8,(int)(r.height+4),WHITE);
    DrawText(TextFormat("%d",val),(int)(r.x+r.width+6),(int)(r.y+1),12,WHITE);
    if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)&&ptInRect(mouse,r)){
        float nt=(mouse.x-r.x)/r.width;
        nt=fmaxf(0.f,fminf(1.f,nt));
        val=mn+(int)roundf(nt*(float)(mx-mn));
    }
    return val;
}

static float drawFloatSlider(Rectangle r,float val,float mn,float mx,
                              const char* label,const char* fmt,Vector2 mouse,Color fill={80,160,80,200}){
    if(mx<=mn) return val; // 1.5: guard division by zero
    if(label&&label[0]) DrawText(label,(int)r.x,(int)(r.y-15),12,C_SECONDARY);
    DrawRectangleRec(r,{18,18,18,255});
    DrawRectangleLinesEx(r,1,{55,55,55,255});
    float t=(val-mn)/(mx-mn);
    DrawRectangle((int)r.x,(int)r.y,(int)(t*r.width),(int)r.height,fill);
    float kx=r.x+t*r.width;
    DrawRectangle((int)(kx-4),(int)(r.y-2),8,(int)(r.height+4),WHITE);
    DrawText(TextFormat(fmt,val),(int)(r.x+r.width+6),(int)(r.y+1),12,WHITE);
    if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)&&ptInRect(mouse,r)){
        float nt=(mouse.x-r.x)/r.width;
        nt=fmaxf(0.f,fminf(1.f,nt));
        val=mn+nt*(mx-mn);
    }
    return val;
}

static void drawSoldierSprite(Texture2D tex,Vector2 pos,float angleDeg,float scale,Color tint,bool drawShadow){
    float s=32.f*scale;
    if(drawShadow){
        Color shad={0,0,0,50};
        DrawCircleV({pos.x+2,pos.y+4},(int)(s*0.3f),shad);
    }
    DrawTexturePro(tex,{0,0,32,32},{pos.x,pos.y,s,s},{s/2,s/2},-angleDeg+90.f,tint);
}

static void drawHealthBar(Vector2 pos,float hp,float maxHp,float w,float scl=1.f){
    if(hp>=maxHp) return;
    float bw=w*scl;
    float ratio=hp/maxHp;
    DrawRectangle((int)(pos.x-bw/2),(int)(pos.y-16*scl),(int)bw,(int)(5*scl),DARKGRAY);
    Color c=ratio>0.5f?Color{0,228,48,255}:ratio>0.25f?Color{253,249,0,255}:Color{230,41,55,255};
    DrawRectangle((int)(pos.x-bw/2),(int)(pos.y-16*scl),(int)(bw*ratio),(int)(5*scl),c);
}

// 3.8: Word-wrapped text helper
static void drawWrappedText(const char* text,int x,int y,int maxWidth,int fontSize,Color col){
    if(!text||!text[0]) return;
    char word[128]; int wi=0;
    char line[512]; int li=0;
    int curX=0;
    auto flushLine=[&](){
        line[li]='\0';
        if(li>0){DrawText(line,x,y,fontSize,col);y+=fontSize+2;}
        li=0; curX=0;
    };
    for(int i=0;;i++){
        char c=text[i];
        if(c==' '||c=='\0'||c=='\n'){
            word[wi]='\0';
            if(wi>0){
                int ww=MeasureText(word,fontSize);
                int spw=(curX>0)?MeasureText(" ",fontSize):0;
                if(curX>0&&curX+spw+ww>maxWidth){
                    flushLine();
                    for(int k=0;k<wi;k++) line[li++]=word[k];
                    curX=ww;
                } else {
                    if(curX>0){line[li++]=' ';curX+=spw;}
                    for(int k=0;k<wi;k++) line[li++]=word[k];
                    curX+=ww;
                }
                wi=0;
            }
            if(c=='\n') flushLine();
            if(c=='\0') break;
        } else {
            if(wi<127) word[wi++]=c;
        }
    }
    flushLine();
}

// Draw stat bars (normalized) for a unit type
static void drawStatBars(float x,float y,float w,float h,const UnitTypeDef& td){
    struct Stat { const char* name; float val, maxv; Color col; };
    Stat stats[]={
        {"HP",     (float)(td.soldierCount)*td.hpPerSoldier, 3000.f, {60,220,60,255}},
        {"Armor",  (float)td.armor,  40.f,  {160,200,220,255}},
        {"Speed",  (float)td.speed, 180.f,  {60,180,220,255}},
        {"M.Atk",  (float)td.meleeAttack, 60.f, {220,160,40,255}},
        {"M.Def",  (float)td.meleeDefense,40.f, {180,120,40,255}},
        {"MelDmg", (float)(td.meleeBaseDmg+td.meleeAPDmg),50.f, {220,80,40,255}},
        {"Range",  (float)td.range, 320.f,  {200,200,80,255}},
        {"MisDmg", (float)(td.missileBaseDmg+td.missileAPDmg),30.f,{200,100,200,255}},
    };
    int n=8;
    float rh=h/n;
    for(int i=0;i<n;i++){
        float fy=y+(float)i*rh;
        DrawText(stats[i].name,(int)x,(int)(fy+1),11,C_SECONDARY);
        float bx=x+60, bw=w-70;
        DrawRectangle((int)bx,(int)(fy+1),(int)bw,(int)(rh-3),{20,20,20,255});
        float ratio=std::min(1.f,stats[i].val/stats[i].maxv);
        DrawRectangle((int)bx,(int)(fy+1),(int)(bw*ratio),(int)(rh-3),stats[i].col);
        DrawText(TextFormat("%.0f",stats[i].val),(int)(bx+bw+4),(int)(fy+1),11,WHITE);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  CAMPAIGN MAP GENERATION
// ═══════════════════════════════════════════════════════════════════════════
static void generateCampaignMap(){
    g_campaign.provinces.clear();

    // 14 provinces, hand-crafted positions normalized to [0,1]
    struct ProvinceTemplate {
        const char* name; float nx,ny; TerrainType terrain; FactionId owner; bool hasCity;
        const char* cityName;
    };
    static const ProvinceTemplate tpls[]={
        {"Aldenmoor",     0.18f,0.42f, TERRAIN_PLAIN,    FACTION_PLAYER,     true,  "Aldenmoor"},
        {"Greenvale",     0.30f,0.28f, TERRAIN_FOREST,   FACTION_PLAYER,     true,  "Greenvale"},
        {"Ironholt",      0.28f,0.62f, TERRAIN_MOUNTAIN, FACTION_NEUTRAL,    true,  "Ironholt"},
        {"Saltmere",      0.14f,0.72f, TERRAIN_COAST,    FACTION_NEUTRAL,    true,  "Saltmere"},
        {"Dustfield",     0.42f,0.50f, TERRAIN_PLAIN,    FACTION_NEUTRAL,    false, ""},
        {"Ashford",       0.52f,0.32f, TERRAIN_FOREST,   FACTION_AGGRESSIVE, true,  "Ashford"},
        {"Cragspire",     0.60f,0.18f, TERRAIN_MOUNTAIN, FACTION_AGGRESSIVE, true,  "Cragspire"},
        {"Redmarch",      0.65f,0.48f, TERRAIN_PLAIN,    FACTION_AGGRESSIVE, true,  "Redmarch"},
        {"Blackfen",      0.50f,0.68f, TERRAIN_FOREST,   FACTION_DEFENSIVE,  true,  "Blackfen"},
        {"Stonewall",     0.72f,0.65f, TERRAIN_MOUNTAIN, FACTION_DEFENSIVE,  true,  "Stonewall"},
        {"Harborgate",    0.35f,0.85f, TERRAIN_COAST,    FACTION_COMMERCIAL, true,  "Harborgate"},
        {"Goldenport",    0.55f,0.82f, TERRAIN_COAST,    FACTION_COMMERCIAL, true,  "Goldenport"},
        {"Thornwood",     0.78f,0.35f, TERRAIN_FOREST,   FACTION_AGGRESSIVE, false, ""},
        {"Highpass",      0.82f,0.52f, TERRAIN_MOUNTAIN, FACTION_DEFENSIVE,  false, ""},
    };
    static const int adjData[][2]={
        {0,1},{0,2},{0,3},{1,4},{1,5},{2,3},{2,4},{2,8},
        {3,10},{4,5},{4,7},{4,8},{5,6},{5,7},{6,7},{6,12},
        {7,8},{7,9},{8,10},{8,11},{9,11},{9,13},{10,11},{12,13}
    };

    int n=14;
    for(int i=0;i<n;i++){
        const ProvinceTemplate& tp=tpls[i];
        Province p{};
        strncpy(p.name,tp.name,sizeof(p.name)-1); p.name[sizeof(p.name)-1]='\0';
        p.center={tp.nx*(float)(SCREEN_W>0?SCREEN_W:1280), tp.ny*(float)(SCREEN_H>0?SCREEN_H:768)};
        p.terrain=tp.terrain; p.owner=tp.owner; p.hasCity=tp.hasCity;
        if(tp.hasCity){
            strncpy(p.city.name,tp.cityName,sizeof(p.city.name)-1); p.city.name[sizeof(p.city.name)-1]='\0';
            memset(p.city.built,0,sizeof(p.city.built));
            p.city.constructing=-1;
            p.city.constructTurns=0;
            p.city.defBonus=1.0f;
            // Player starter city has barracks + farm
            if(tp.owner==FACTION_PLAYER){
                p.city.built[BLD_FARM]=true;
                p.city.built[BLD_BARRACKS]=true;
            }
            // Enemy cities have some buildings
            if(tp.owner==FACTION_AGGRESSIVE||tp.owner==FACTION_DEFENSIVE){
                p.city.built[BLD_FARM]=true;
                p.city.built[BLD_BARRACKS]=true;
            }
            if(tp.owner==FACTION_COMMERCIAL){
                p.city.built[BLD_FARM]=true;
                p.city.built[BLD_MARKET]=true;
            }
        }
        p.army.clear();
        // Add enemy armies
        if(tp.owner==FACTION_AGGRESSIVE){
            p.army.push_back({0,40}); // Spear Levy x40
            p.army.push_back({1,30}); // Axe Militia x30
        } else if(tp.owner==FACTION_DEFENSIVE){
            p.army.push_back({0,50});
            p.army.push_back({4,20}); // Bow Levy
        } else if(tp.owner==FACTION_COMMERCIAL){
            p.army.push_back({0,30});
        }
        g_campaign.provinces.push_back(p);
    }
    // Add adjacencies
    for(auto& pr:g_campaign.provinces) pr.adjacent.clear();
    for(auto& ad:adjData){
        if(ad[0]<n&&ad[1]<n){
            g_campaign.provinces[ad[0]].adjacent.push_back(ad[1]);
            g_campaign.provinces[ad[1]].adjacent.push_back(ad[0]);
        }
    }
    // Update center coords from actual screen size
}

static void updateProvinceCenters(){
    static const float nxs[]={0.18f,0.30f,0.28f,0.14f,0.42f,0.52f,0.60f,0.65f,0.50f,0.72f,0.35f,0.55f,0.78f,0.82f};
    static const float nys[]={0.42f,0.28f,0.62f,0.72f,0.50f,0.32f,0.18f,0.48f,0.68f,0.65f,0.85f,0.82f,0.35f,0.52f};
    for(int i=0;i<(int)g_campaign.provinces.size();i++){
        g_campaign.provinces[i].center={nxs[i]*SCREEN_W, nys[i]*SCREEN_H};
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  NEW CAMPAIGN INIT
// ═══════════════════════════════════════════════════════════════════════════
static void newCampaign(){
    g_campaign=CampaignState{};
    g_campaign.res.gold=500.f;
    g_campaign.res.food=200.f;
    g_campaign.res.wood=100.f;
    g_campaign.res.stone=50.f;
    g_campaign.res.iron=50.f;
    g_campaign.playerProvince=0;
    generateCampaignMap();
    updateProvinceCenters();
    // Player starts with 3 spear levy groups
    g_campaign.playerArmy.push_back({0,60});
    g_campaign.playerArmy.push_back({0,60});
    g_campaign.playerArmy.push_back({4,40}); // Bow levy
    g_campaign.readyUnits=g_campaign.playerArmy;
    g_campaign.pendingBattleProvince=-1;
    g_campaign.turn=1;
}

// ═══════════════════════════════════════════════════════════════════════════
//  TURN PROCESSING (centralized)
// ═══════════════════════════════════════════════════════════════════════════
static void processTurn(){
    Resources& r=g_campaign.res;

    // Advance recruitment queue
    for(auto& e:g_campaign.recruitQueue){
        e.turnsLeft--;
        if(e.turnsLeft<=0){
            g_campaign.readyUnits.push_back({e.typeIdx,e.typeIdx<(int)g_unitTypes.size()?g_unitTypes[e.typeIdx].soldierCount:40});
            battleLogAdd(TextFormat("[T%d] %s recruitment complete!", g_campaign.turn, g_unitTypes[e.typeIdx].name));
        }
    }
    // Remove completed recruitment
    g_campaign.recruitQueue.erase(
        std::remove_if(g_campaign.recruitQueue.begin(), g_campaign.recruitQueue.end(),
            [](const RecruitEntry& e){ return e.turnsLeft<=0; }),
        g_campaign.recruitQueue.end()
    );

    // Resource generation from ALL player cities
    for(int pi=0;pi<(int)g_campaign.provinces.size();pi++){
        Province& prov=g_campaign.provinces[pi];
        if(prov.owner!=FACTION_PLAYER||!prov.hasCity) continue;
        City& c=prov.city;
        for(int b=0;b<BLD_COUNT;b++){
            if(!c.built[b]) continue;
            r.gold+=bldGoldYield[b];
            r.food+=bldFoodYield[b];
            r.wood+=bldWoodYield[b];
            r.stone+=bldStoneYield[b];
            r.iron+=bldIronYield[b];
        }
        // Advance construction
        if(c.constructing>=0){
            c.constructTurns--;
            if(c.constructTurns<=0){
                c.built[c.constructing]=true;
                battleLogAdd(TextFormat("[T%d] %s: %s completed!", g_campaign.turn, prov.name, bldNames[c.constructing]));
                c.constructing=-1;
            }
        }
    }

    // Maintenance from all units
    for(auto [ti,cnt]:g_campaign.readyUnits){
        if(ti>=unitTypeCount()) continue;
        const UnitTypeDef& td=g_unitTypes[ti];
        r.gold-=td.maintGold*(cnt/(float)td.soldierCount);
        r.food-=td.maintFood*(cnt/(float)td.soldierCount);
    }

    // Clamp resources (don't go negative)
    r.gold=std::max(0.f,r.gold);
    r.food=std::max(0.f,r.food);
    r.wood=std::max(0.f,r.wood);
    r.stone=std::max(0.f,r.stone);
    r.iron=std::max(0.f,r.iron);

    // Enemy AI and actions
    float aggression=0.05f+0.02f*(float)g_campaign.turn/10.f;
    aggression=std::min(aggression,0.4f);
    static const float diffMult[3]={0.7f,1.0f,1.4f};
    aggression*=diffMult[std::max(0,std::min(2,g_settings.difficulty))];

    // Coalition detection
    std::vector<int> coalitionAttackers;
    for(int pi2=0;pi2<(int)g_campaign.provinces.size();pi2++){
        Province& ep2=g_campaign.provinces[pi2];
        if(ep2.owner==FACTION_NEUTRAL||ep2.owner==FACTION_PLAYER) continue;
        bool adjPlayer=false;
        for(int adj2:ep2.adjacent) if(g_campaign.provinces[adj2].owner==FACTION_PLAYER) adjPlayer=true;
        if(!adjPlayer) continue;
        for(int adj2:ep2.adjacent){
            Province& ep3=g_campaign.provinces[adj2];
            if(ep3.owner!=ep2.owner) continue;
            bool partnerAdjPlayer=false;
            for(int adj3:ep3.adjacent) if(g_campaign.provinces[adj3].owner==FACTION_PLAYER) partnerAdjPlayer=true;
            if(partnerAdjPlayer){
                coalitionAttackers.push_back(pi2);
                break;
            }
        }
    }

    for(int pi2=0;pi2<(int)g_campaign.provinces.size();pi2++){
        Province& ep=g_campaign.provinces[pi2];
        if(ep.owner==FACTION_NEUTRAL||ep.owner==FACTION_PLAYER) continue;

        float roll=frandMT();
        bool inCoalition=false;
        for(int ca:coalitionAttackers) if(ca==pi2){inCoalition=true;break;}
        float attackChance=inCoalition?aggression*2.f:aggression;

        if(roll<attackChance*0.4f){
            // Attack weak neighbor
            for(int adj:ep.adjacent){
                if(g_campaign.provinces[adj].owner==FACTION_PLAYER){
                    int eStr=0; for(auto [ti,c]:ep.army) eStr+=c;
                    int pStr=(int)g_campaign.readyUnits.size()*40;
                    if(eStr>(int)(pStr*1.2f)){
                        g_preBattle.provinceIdx=adj;
                        g_preBattle.isDefense=true;
                        g_preBattle.fogOfWar=false;
                        g_preBattle.estimatedEnemyStrength=eStr;
                        g_preBattle.include.clear();
                        g_preBattle.include.resize(g_campaign.readyUnits.size(),true);
                        for(int j=12;j<(int)g_preBattle.include.size();j++)
                            g_preBattle.include[j]=false;
                        g_campaign.provinces[adj].army=ep.army;
                        g_campaign.battlesLost=g_campaign.battlesLost;
                        // Don't return here - we'll handle battle after incrementing turn
                    }
                    break;
                }
            }
        } else if(roll<attackChance*0.4f+0.2f){
            // Reinforcement transfer
            for(int adj:ep.adjacent){
                Province& ap=g_campaign.provinces[adj];
                if(ap.owner!=ep.owner||ap.army.empty()) continue;
                int apStr=0; for(auto [ti,c]:ap.army) apStr+=c;
                int epStr=0; for(auto [ti,c]:ep.army) epStr+=c;
                if(apStr>epStr+20&&!ap.army.empty()){
                    auto& transferUnit=ap.army.front();
                    int transferAmt=transferUnit.second/4;
                    if(transferAmt>0){
                        bool found=false;
                        for(auto& eu:ep.army) if(eu.first==transferUnit.first){eu.second+=transferAmt;found=true;break;}
                        if(!found) ep.army.push_back({transferUnit.first,transferAmt});
                        transferUnit.second-=transferAmt;
                        if(transferUnit.second<=0) ap.army.erase(ap.army.begin());
                    }
                    break;
                }
            }
        }
    }

    // Check victory condition
    int totalProv=(int)g_campaign.provinces.size();
    int playerProv=0;
    for(auto& p:g_campaign.provinces) if(p.owner==FACTION_PLAYER) playerProv++;
    g_campaign.peakProvinces=std::max(g_campaign.peakProvinces,playerProv);
    if(playerProv>=(int)(totalProv*0.8f)){
        battleLogAdd("[Campaign] VICTORY! Empire established!");
    }
    if(playerProv==0){
        battleLogAdd("[Campaign] DEFEAT — all provinces lost!");
    }

    // Finally: Increment turn after all processing
    g_campaign.turn++;
}

// ═══════════════════════════════════════════════════════════════════════════
//  BATTLE FORMATION HELPERS (v7.0: LINE FORMATION - rectangular 360° rotation)
// ═══════════════════════════════════════════════════════════════════════════
static std::vector<Vector2> calcFormationSlots(Vector2 anchor,int count,float facing){
    std::vector<Vector2> slots;

    // LINE FORMATION: wide rectangular front (width >> depth)
    // Creates a proper battle line that soldiers will maintain throughout combat
    int cols=(int)ceilf(sqrtf((float)count*2.0f)); // wider lines (2.0 ratio for wider front)
    int rows=(int)ceilf((float)count/(float)cols);

    float spacing=15.f;  // tight spacing for compact line formation

    // Facing direction vector (0° = right, 90° = down, 180° = left, 270° = up)
    float rad=facing*DEG2RAD;
    Vector2 fwd={cosf(rad),-sinf(rad)};      // forward direction
    Vector2 right={-fwd.y,fwd.x};             // right direction (perpendicular)

    // Generate slots: soldiers arranged in rows perpendicular to facing direction
    // This creates a rectangular line formation that rotates properly
    for(int r=0;r<rows;r++){
        for(int c=0;c<cols;c++){
            int idx=r*cols+c;
            if(idx>=count) break;

            // Offset from anchor:
            // - lateral (ox): across the front width
            // - depth (oy): depth into the formation
            float ox=(c-(cols-1)*0.5f)*spacing;  // lateral spacing across front
            float oy=r*spacing*0.85f;             // depth spacing (slightly compressed)

            // Calculate slot position: move from anchor along right and forward axes
            Vector2 slot={
                anchor.x + right.x*ox + fwd.x*oy,
                anchor.y + right.y*ox + fwd.y*oy
            };
            slots.push_back(slot);
        }
    }
    return slots;
}

// ═══════════════════════════════════════════════════════════════════════════
//  BATTLE INIT
// ═══════════════════════════════════════════════════════════════════════════
static void initBattle(const std::vector<std::pair<int,int>>& playerGroups,
                        const std::vector<std::pair<int,int>>& enemyGroups,
                        TerrainType terrain,int provinceIdx,bool isDefense,
                        const char* scenarioName){
    g_battle={};
    g_battle.terrain=terrain;
    g_battle.battleProvince=provinceIdx;
    g_battle.isDefense=isDefense;
    g_battle.timeScale=1.f;
    strncpy(g_battle.scenarioName,scenarioName,sizeof(g_battle.scenarioName)-1); g_battle.scenarioName[sizeof(g_battle.scenarioName)-1]='\0';

    // Camera centered on player deployment zone
    g_battle.cam.offset={SCREEN_W/2.f,(SCREEN_H-HUD_H)/2.f};
    g_battle.cam.target={BATTLE_W*0.25f,BATTLE_H/2.f};
    g_battle.cam.rotation=0;
    g_battle.cam.zoom=1.f;
    g_battle.camZoom=1.f;

    // Generate terrain obstacles
    g_battle.obstacles.clear();
    if(terrain==TERRAIN_FOREST){
        // Clusters of trees as obstacle rects
        srand(provinceIdx*13337);
        for(int i=0;i<25;i++){
            float ox=(float)(rand()%BATTLE_W);
            float oy=(float)(rand()%BATTLE_H);
            float ow=40.f+(float)(rand()%60);
            float oh=40.f+(float)(rand()%60);
            g_battle.obstacles.push_back({ox,oy,ow,oh});
        }
    } else if(terrain==TERRAIN_MOUNTAIN){
        srand(provinceIdx*7777);
        for(int i=0;i<12;i++){
            float ox=(float)(rand()%BATTLE_W);
            float oy=(float)(rand()%BATTLE_H);
            g_battle.obstacles.push_back({ox,oy,80.f+rand()%120,80.f+rand()%120});
        }
    }
    srand((unsigned)time(nullptr));

    // Player units — left side
    float py=BATTLE_H/2.f;
    float px=BATTLE_W*0.2f;
    for(int gi=0;gi<(int)playerGroups.size();gi++){
        auto [typeIdx,cnt]=playerGroups[gi];
        if(typeIdx>=unitTypeCount()||cnt<=0) continue;
        const UnitTypeDef& td=g_unitTypes[typeIdx];
        BattleUnit bu{};
        bu.typeIdx=typeIdx; bu.isPlayer=true;
        bu.anchorPos={px, py+(gi-playerGroups.size()/2.f)*90.f};
        bu.orderTarget=bu.anchorPos;
        bu.orderAttack=-1;
        bu.morale=td.morale;
        bu.groupState=UGS_IDLE;
        bu.veterancy=0;
        bu.aiReactTimer=0.f;
        bu.orderType=0; // 6.6: attack order type
        bu.formationFacing=0.f; // 6.6: facing angle (0 = right)
        bu.lastFormationFacing=0.f; // v7.2: initialize facing tracker
        int soldierCount=std::min(cnt,(int)td.soldierCount);
        auto slots=calcFormationSlots(bu.anchorPos,soldierCount,0.f);
        for(int s=0;s<soldierCount;s++){
            Soldier sol{};
            sol.pos=slots[s];
            sol.formationSlot=slots[s];
            sol.hp=td.hpPerSoldier;
            sol.alive=true;
            sol.angle=0; sol.angleTarget=0;
            sol.attackTargetSoldier=-1; sol.attackTargetUnit=-1;
            sol.state=SS_IDLE;
            sol.chargeReady=(td.spriteBase==SPR_CAVALRY);
            sol.targetRefreshTimer=0.f;
            sol.kills=0;
            bu.soldiers.push_back(sol);
        }
        g_battle.playerUnits.push_back(bu);
    }

    // Enemy units — right side
    float ey=BATTLE_H/2.f;
    float ex2=BATTLE_W*0.8f;
    for(int gi=0;gi<(int)enemyGroups.size();gi++){
        auto [typeIdx,cnt]=enemyGroups[gi];
        if(typeIdx>=unitTypeCount()||cnt<=0) continue;
        const UnitTypeDef& td=g_unitTypes[typeIdx];
        BattleUnit bu{};
        bu.typeIdx=typeIdx; bu.isPlayer=false;
        bu.anchorPos={ex2, ey+(gi-(int)enemyGroups.size()/2.f)*90.f};
        bu.orderTarget=bu.anchorPos;
        bu.orderAttack=-1;
        bu.morale=td.morale;
        bu.groupState=UGS_IDLE;
        bu.veterancy=0;
        bu.aiReactTimer=0.f;
        bu.orderType=0; // 6.6: attack order type
        bu.formationFacing=180.f; // 6.6: facing angle (180 = left for enemies)
        bu.lastFormationFacing=180.f; // v7.2: initialize facing tracker
        int soldierCount=std::min(cnt,(int)td.soldierCount);
        auto slots=calcFormationSlots(bu.anchorPos,soldierCount,180.f);
        for(int s=0;s<soldierCount;s++){
            Soldier sol{};
            sol.pos=slots[s];
            sol.formationSlot=slots[s];
            sol.hp=td.hpPerSoldier;
            sol.alive=true;
            sol.angle=180; sol.angleTarget=180;
            sol.attackTargetSoldier=-1; sol.attackTargetUnit=-1;
            sol.state=SS_IDLE;
            sol.chargeReady=(td.spriteBase==SPR_CAVALRY);
            sol.targetRefreshTimer=0.f;
            sol.kills=0;
            bu.soldiers.push_back(sol);
        }
        g_battle.enemyUnits.push_back(bu);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  COMBAT HELPERS
// ═══════════════════════════════════════════════════════════════════════════
static float soldierMeleeRadius(const UnitTypeDef& td){
    return td.spriteBase==SPR_CAVALRY?16.f:10.f;
}

static float calcMeleeDmg(const UnitTypeDef& attTd,const UnitTypeDef& defTd,bool chargeBonus){
    float hitChance=std::max(8.f,std::min(92.f,30.f+(float)attTd.meleeAttack-(float)defTd.meleeDefense));
    if(frandMT()*100.f>=hitChance) return 0.f;
    float armRed=frandMT()*0.5f*(float)defTd.armor+(float)defTd.armor*0.5f;
    armRed=std::min(armRed,(float)defTd.armor);
    float dmg=std::max(0.5f,attTd.meleeBaseDmg*(1.f-armRed/100.f)+attTd.meleeAPDmg);
    if(chargeBonus) dmg*=2.5f;
    return dmg;
}

static float calcMissileDmg(const UnitTypeDef& attTd,const UnitTypeDef& defTd){
    float armRed=frandMT()*0.5f*(float)defTd.armor+(float)defTd.armor*0.5f;
    armRed=std::min(armRed,(float)defTd.armor);
    return std::max(0.f,attTd.missileBaseDmg*(1.f-armRed/100.f)+attTd.missileAPDmg);
}

// 6.6: Improved obstacle avoidance with steering behavior
static Vector2 avoidObstacles(Vector2 pos, Vector2 targetDir, float speed, const std::vector<Rectangle>& obstacles){
    Vector2 ahead=v2add(pos,v2scale(targetDir,speed*0.5f));
    Vector2 ahead2=v2add(pos,v2scale(targetDir,speed*0.25f));

    // Check if we're hitting an obstacle
    Rectangle checkBox={ahead.x-8,ahead.y-8,16,16};
    Rectangle checkBox2={ahead2.x-8,ahead2.y-8,16,16};

    for(auto& obs:obstacles){
        if(CheckCollisionRecs(checkBox,obs)||CheckCollisionRecs(checkBox2,obs)){
            // Try to steer around obstacle
            Vector2 obsCenter={obs.x+obs.width/2,obs.y+obs.height/2};
            Vector2 toObs=vnorm(v2sub(obsCenter,pos));
            Vector2 perp={-toObs.y,toObs.x};
            // Steer perpendicular to obstacle
            return vnorm(v2add(targetDir,v2scale(perp,0.7f)));
        }
    }
    return targetDir;
}

// LOS check for ranged: returns true if line is clear enough to shoot
static bool losClean(Vector2 from,Vector2 to,const std::vector<Soldier>& friendlies,int selfIdx){
    Vector2 dir=vnorm(v2sub(to,from));
    float dist=vdist(from,to);
    int blocked=0;
    for(int i=0;i<(int)friendlies.size();i++){
        if(i==selfIdx||!friendlies[i].alive) continue;
        Vector2 rel=v2sub(friendlies[i].pos,from);
        float proj=v2dot(rel,dir);
        if(proj<0||proj>dist) continue;
        float perp=fabsf(rel.x*(-dir.y)+rel.y*dir.x);
        if(perp<10.f){ blocked++; if(blocked>=2) return false; }
    }
    // Check terrain obstacles
    for(auto& obs:g_battle.obstacles){
        // Simple: check if line intersects rectangle
        // Rough check: if rect centroid is within 30px of line
        Vector2 rc={obs.x+obs.width/2,obs.y+obs.height/2};
        Vector2 rel=v2sub(rc,from);
        float proj=v2dot(rel,dir);
        if(proj>0&&proj<dist){
            float perp=fabsf(rel.x*(-dir.y)+rel.y*dir.x);
            if(perp<(obs.width+obs.height)*0.25f) return false;
        }
    }
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
//  BATTLE UPDATE
// ═══════════════════════════════════════════════════════════════════════════
static void separateSoldiers(std::vector<BattleUnit>& units){
    // Separation within same side
    float minD=18.f;
    for(auto& bu:units){
        for(int i=0;i<(int)bu.soldiers.size();i++){
            if(!bu.soldiers[i].alive) continue;
            // Against same unit
            for(int j=i+1;j<(int)bu.soldiers.size();j++){
                if(!bu.soldiers[j].alive) continue;
                float d=vdist(bu.soldiers[i].pos,bu.soldiers[j].pos);
                if(d<minD&&d>0.001f){
                    float push=(minD-d)*0.35f;
                    Vector2 dir2=vnorm(v2sub(bu.soldiers[j].pos,bu.soldiers[i].pos));
                    bu.soldiers[i].pos=v2sub(bu.soldiers[i].pos,v2scale(dir2,push));
                    bu.soldiers[j].pos=v2add(bu.soldiers[j].pos,v2scale(dir2,push));
                }
            }
            // Clamp
            bu.soldiers[i].pos.x=std::max(8.f,std::min((float)BATTLE_W-8,bu.soldiers[i].pos.x));
            bu.soldiers[i].pos.y=std::max(8.f,std::min((float)BATTLE_H-8,bu.soldiers[i].pos.y));
        }
    }
}

// Find nearest alive soldier in an enemy BattleUnit list, returns {unitIdx, solIdx} or {-1,-1}
static std::pair<int,int> findNearestEnemySoldier(Vector2 from,
        const std::vector<BattleUnit>& enemies){
    float best=1e9f; int bu2=-1,si=-1;
    for(int u=0;u<(int)enemies.size();u++){
        for(int s=0;s<(int)enemies[u].soldiers.size();s++){
            if(!enemies[u].soldiers[s].alive) continue;
            float d=vdist(from,enemies[u].soldiers[s].pos);
            if(d<best){best=d;bu2=u;si=s;}
        }
    }
    return {bu2,si};
}

static void updateBattleUnits(std::vector<BattleUnit>& myUnits,
                               std::vector<BattleUnit>& foeUnits,
                               float dt){
    for(int ui=0;ui<(int)myUnits.size();ui++){
        BattleUnit& bu=myUnits[ui];
        if(bu.typeIdx>=unitTypeCount()) continue;
        const UnitTypeDef& td=g_unitTypes[bu.typeIdx];

        int aliveCnt=0;
        for(auto& sol:bu.soldiers) if(sol.alive) aliveCnt++;
        if(aliveCnt==0) continue;

        // Morale update
        float totalSol=(float)td.soldierCount;
        if((float)aliveCnt/totalSol < 0.4f){
            bu.morale-=1.f*dt;
        }
        bu.moraleTimer+=dt;
        if(bu.moraleTimer>4.f&&bu.morale<td.morale){
            bu.morale=std::min(td.morale,bu.morale+0.5f*60.f*dt);
        }
        bu.morale=std::max(0.f,std::min(100.f,bu.morale));

        // Routing logic
        if(bu.groupState==UGS_ROUTING){
            bu.routeTimer-=dt;
            if(bu.routeTimer<=0){
                if(bu.morale<5.f){
                    // Permanently rout — kill all soldiers
                    for(auto& sol:bu.soldiers) sol.alive=false;
                } else {
                    bu.groupState=UGS_IDLE;
                }
            }
            // Soldiers flee
            // Compute enemy centroid
            Vector2 foeCenter={0,0}; int foeCount=0;
            for(auto& fu:foeUnits) for(auto& fs:fu.soldiers) if(fs.alive){foeCenter=v2add(foeCenter,fs.pos);foeCount++;}
            if(foeCount>0){foeCenter=v2scale(foeCenter,1.f/foeCount);}
            for(auto& sol:bu.soldiers){
                if(!sol.alive) continue;
                sol.state=SS_FLEEING;
                Vector2 away=vnorm(v2sub(sol.pos,foeCenter));
                sol.pos=v2add(sol.pos,v2scale(away,td.speed*1.5f*dt));
                sol.angleTarget=dirToAngle(away);
                sol.angle=lerpAngle(sol.angle,sol.angleTarget,8.f*dt);
            }
            continue;
        }
        if(bu.morale<20.f&&bu.groupState!=UGS_ROUTING){
            bu.groupState=UGS_ROUTING;
            bu.routeTimer=6.f;
        }

        // ═══════════════════════════════════════════════════════════════════════════
        //  MOVEMENT SYSTEM v8.1: ANCHOR FOLLOWS CENTROID (STABLE)
        // ═══════════════════════════════════════════════════════════════════════════

        // Determine group order target (explicit attack or move)
        bool hasAttackOrder=(bu.orderAttack>=0&&bu.orderAttack<(int)foeUnits.size());
        bool hasExplicitMoveOrder=(bu.hasExplicitOrder&&bu.orderType==1);

        // Calculate desired movement (toward order target or auto-advance to enemies)
        Vector2 desiredAnchor=bu.anchorPos;

        if(hasExplicitMoveOrder){
            Vector2 moveDir=vnorm(v2sub(bu.orderTarget,bu.anchorPos));
            float distToTarget=vdist(bu.anchorPos,bu.orderTarget);

            if(distToTarget>15.f){
                desiredAnchor=v2add(bu.anchorPos,v2scale(moveDir,td.speed*dt));
                bu.formationFacing=dirToAngle(moveDir);
            } else {
                // Arrived at target
                desiredAnchor=bu.orderTarget;
                bu.hasExplicitOrder=false;
            }
        } 
        else if(hasAttackOrder||(!hasExplicitMoveOrder&&!bu.hasExplicitOrder&&foeUnits.size()>0)){
            // Auto-advance toward nearest enemy when no explicit orders (v8.2: SPEED OPTIMIZED)
            // FIX: find nearest enemy group, not always index 0
            int targetIdx=hasAttackOrder?bu.orderAttack:0;
            if(!hasAttackOrder){
                float bestDist=1e9f;
                for(int ei=0;ei<(int)foeUnits.size();ei++){
                    int eAlive=0; for(auto& es:foeUnits[ei].soldiers) if(es.alive) eAlive++;
                    if(eAlive==0) continue;
                    float d=vdist(bu.anchorPos,foeUnits[ei].anchorPos);
                    if(d<bestDist){bestDist=d;targetIdx=ei;}
                }
            }
            if(validIdx(targetIdx,foeUnits)){
                Vector2 toEnemy=vnorm(v2sub(foeUnits[targetIdx].anchorPos,bu.anchorPos));
                float distToEnemy=vdist(bu.anchorPos,foeUnits[targetIdx].anchorPos);

                // Move toward enemy at full speed until close engagement (v8.2: AGGRESSIVE)
                // This prevents units from orbiting instead of engaging
                float engagementDistance=70.f; // Reduced from 100px for faster engagement
                if(distToEnemy>engagementDistance){
                    desiredAnchor=v2add(bu.anchorPos,v2scale(toEnemy,td.speed*dt));
                }

                // Rotate formation to face enemy
                bu.formationFacing=dirToAngle(toEnemy);
            }
        }

        // FIX: ALWAYS recalculate slots using desiredAnchor (the intended new position).
        // The old guard (>5 degree facing change) caused slots to go stale during straight-line
        // movement: anchor advanced but soldiers chased old world positions → wrong direction / trembling.
        // formationSlot is an absolute world position, so it MUST update whenever the anchor moves.
        {
            bu.lastFormationFacing=bu.formationFacing;
            auto slots=calcFormationSlots(desiredAnchor,(int)bu.soldiers.size(),bu.formationFacing);
            for(int s=0;s<(int)bu.soldiers.size()&&s<(int)slots.size();s++){
                bu.soldiers[s].formationSlot=slots[s];
            }
        }

        // ═══════════════════════════════════════════════════════════════════════════
        //  INDIVIDUAL SOLDIER UPDATES (v8.0: SMOOTH GRAVITATIONAL PULL)
        // ═══════════════════════════════════════════════════════════════════════════

        // Calculate frontline threshold ONCE per unit (optimization + stability)
        float frontlineDistance=1e9f;
        for(auto& eneunit:foeUnits){
            for(auto& esol:eneunit.soldiers){
                if(!esol.alive) continue;
                for(auto& sol:bu.soldiers){
                    if(!sol.alive) continue;
                    float d=vdist(sol.pos,esol.pos);
                    frontlineDistance=std::min(frontlineDistance,d);
                }
            }
        }
        if(frontlineDistance==1e9f) frontlineDistance=200.f;
        float frontlineThreshold=frontlineDistance+50.f;

        // Each soldier updates individually
        for(int si=0;si<(int)bu.soldiers.size();si++){
            Soldier& sol=bu.soldiers[si];
            if(!sol.alive) continue;

            sol.meleeTimer-=dt;
            sol.rangeTimer-=dt;
            sol.inMelee=false;

            // Check if this soldier is in front line
            float distToClosestEnemy=1e9f;
            for(auto& eneunit:foeUnits){
                for(auto& esol:eneunit.soldiers){
                    if(!esol.alive) continue;
                    float d=vdist(sol.pos,esol.pos);
                    distToClosestEnemy=std::min(distToClosestEnemy,d);
                }
            }
            bool inFrontLine=(distToClosestEnemy<=frontlineThreshold);

            // Resolve attack target
            int targetUnit=sol.attackTargetUnit;
            int targetSol=sol.attackTargetSoldier;
            bool targetValid=(targetUnit>=0&&targetUnit<(int)foeUnits.size()
                              &&targetSol>=0&&targetSol<(int)foeUnits[targetUnit].soldiers.size()
                              &&foeUnits[targetUnit].soldiers[targetSol].alive);

            if(!targetValid){
                sol.targetRefreshTimer-=dt;
                bool needRefresh=(sol.targetRefreshTimer<=0.f);
                if(!needRefresh&&sol.attackTargetUnit>=0){
                    if(validIdx(sol.attackTargetUnit,foeUnits)&&
                       validIdx(sol.attackTargetSoldier,foeUnits[sol.attackTargetUnit].soldiers)&&
                       foeUnits[sol.attackTargetUnit].soldiers[sol.attackTargetSoldier].alive){
                        targetUnit=sol.attackTargetUnit;
                        targetSol=sol.attackTargetSoldier;
                        targetValid=true;
                    } else needRefresh=true;
                }
                if(needRefresh){
                    sol.targetRefreshTimer=0.3f;
                    if(hasAttackOrder){
                        float best=1e9f; int bs=-1;
                        for(int s2=0;s2<(int)foeUnits[bu.orderAttack].soldiers.size();s2++){
                            if(!foeUnits[bu.orderAttack].soldiers[s2].alive) continue;
                            float d=vdist(sol.pos,foeUnits[bu.orderAttack].soldiers[s2].pos);
                            if(d<best){best=d;bs=s2;}
                        }
                        if(bs>=0){targetUnit=bu.orderAttack;targetSol=bs;targetValid=true;}
                    }
                    if(!targetValid&&!bu.hasExplicitOrder){
                        auto [nu,ns]=findNearestEnemySoldier(sol.pos,foeUnits);
                        if(nu>=0){targetUnit=nu;targetSol=ns;targetValid=true;}
                    }
                    sol.attackTargetUnit=targetUnit;
                    sol.attackTargetSoldier=targetSol;
                }
            }

            // ═══════════════════════════════════════════════════════════════════════════
            //  SOLDIER MOVEMENT PRIORITY SYSTEM v8.0
            // ═══════════════════════════════════════════════════════════════════════════

            float slotDist=vdist(sol.pos,sol.formationSlot);
            const float SLOT_TOLERANCE=8.f; // When soldier is "in slot" (strict precision)

            // If no target, move to formation slot instead of idle
            if(!targetValid){
                sol.state=SS_MOVING_SLOT;

                if(slotDist>SLOT_TOLERANCE){
                    Vector2 toSlot=vnorm(v2sub(sol.formationSlot,sol.pos));
                    toSlot=avoidObstacles(sol.pos,toSlot,td.speed,g_battle.obstacles);
                    // v8.2: Fast movement - linear instead of deceleration curve
                    // Soldiers move at FULL speed toward their slot
                    sol.pos=v2add(sol.pos,v2scale(toSlot,td.speed*dt));
                    sol.angleTarget=dirToAngle(toSlot);
                    sol.chargeMoveTime+=dt;
                    if(sol.chargeMoveTime>0.8f) sol.chargeReady=(td.spriteBase==SPR_CAVALRY);
                } else {
                    sol.state=SS_IDLE;
                    sol.chargeMoveTime=0.f;
                }
                sol.angle=lerpAngle(sol.angle,sol.angleTarget,10.f*dt);
                continue;  // Skip combat logic if no target
            }

            Soldier& tgt=foeUnits[targetUnit].soldiers[targetSol];
            const UnitTypeDef& fTd=g_unitTypes[foeUnits[targetUnit].typeIdx];
            float myRad=soldierMeleeRadius(td);
            float fRad=soldierMeleeRadius(fTd);
            float meleeD=myRad+fRad+6.f;
            float dist=vdist(sol.pos,tgt.pos);
            Vector2 dir2=vnorm(v2sub(tgt.pos,sol.pos));
            sol.angleTarget=dirToAngle(dir2);

            // Priority 1: MELEE COMBAT (front line only) - BREAK FORMATION
            if(inFrontLine&&dist<=meleeD){
                sol.inMelee=true;
                sol.state=SS_ATTACKING_MELEE;

                // v8.2: Soldiers in melee BREAK FORMATION and fight freely around target
                // FIX: radius fixed per soldier index (no random per frame), time from GetTime()
                float orbitRadius=20.f+(float)(si%4)*4.f;  // 20-32px, stable per soldier
                float orbitSpeed=2.5f;  // radians/second orbital speed

                // FIX: use GetTime() for actual elapsed time, not g_campaign.turn (static int)
                float timeFactor=(float)GetTime() + (float)si*0.7f;  // unique phase per soldier
                float orbitAngle=timeFactor*orbitSpeed;
                Vector2 orbitPos={
                    tgt.pos.x + cosf(orbitAngle)*orbitRadius,
                    tgt.pos.y + sinf(orbitAngle)*orbitRadius
                };

                // Move toward orbital position (not rigid slot)
                Vector2 toOrbit=vnorm(v2sub(orbitPos,sol.pos));
                sol.pos=v2add(sol.pos,v2scale(toOrbit,td.speed*0.4f*dt));  // Slower movement in melee

                if(sol.meleeTimer<=0){
                    bool chargeBonus=sol.chargeReady&&td.spriteBase==SPR_CAVALRY;
                    float dmg=calcMeleeDmg(td,fTd,chargeBonus);
                    if(chargeBonus&&dmg>0){
                        sol.chargeReady=false;
                        tgt.pos=v2add(tgt.pos,v2scale(dir2,12.f));
                        foeUnits[targetUnit].morale-=10.f;
                    }
                    tgt.hp-=dmg;
                    if(tgt.hp<=0){
                        tgt.alive=false;
                        sol.kills++;
                        g_battle.dead.push_back({tgt.pos,1.f,8.f,fTd.spriteBase==SPR_CAVALRY});
                        if(fTd.soldierCount>0){
                            char logbuf[128];
                            snprintf(logbuf,127,"[T%d] %s soldier slain by %s",
                                g_campaign.turn,fTd.name,td.name);
                            battleLogAdd(logbuf);
                        }
                        for(int fui=0;fui<(int)foeUnits.size();fui++){
                            for(auto& fs:foeUnits[fui].soldiers){
                                if(!fs.alive) continue;
                                if(vdist(tgt.pos,fs.pos)<80.f) foeUnits[fui].morale-=3.f;
                            }
                        }
                        sol.attackTargetSoldier=-1; sol.attackTargetUnit=-1;
                        sol.targetRefreshTimer=0.f;
                    }
                    sol.meleeTimer=td.meleeInterval;
                }
                sol.chargeMoveTime=0.f;
            }
            // Priority 2: RANGED COMBAT (back line support, not in melee)
            else if(td.range>0&&dist<=(float)td.range&&!sol.inMelee&&sol.rangeTimer<=0){
                if(losClean(sol.pos,tgt.pos,bu.soldiers,si)){
                    sol.state=SS_ATTACKING_RANGED;
                    float dmg=calcMissileDmg(td,fTd);
                    bool isCross=(td.missileReload>2.5f);
                    float projSpeed=isCross?380.f:320.f;
                    g_battle.projs.push_back({sol.pos,v2scale(dir2,projSpeed),dmg,true,bu.isPlayer,isCross});
                    sol.rangeTimer=td.missileReload;
                    sol.chargeMoveTime=0.f;
                }
            }
            // Priority 3: MOVE TOWARD SLOT IF ENEMY OUT OF RANGE
            else {
                sol.state=SS_MOVING_SLOT;
                float slotDist=vdist(sol.pos,sol.formationSlot);

                if(slotDist>8.f){
                    Vector2 toSlot=vnorm(v2sub(sol.formationSlot,sol.pos));
                    toSlot=avoidObstacles(sol.pos,toSlot,td.speed,g_battle.obstacles);
                    // v8.2: Fast movement - no deceleration curve
                    sol.pos=v2add(sol.pos,v2scale(toSlot,td.speed*dt));
                    sol.angleTarget=dirToAngle(toSlot);
                    sol.chargeMoveTime+=dt;
                    if(sol.chargeMoveTime>0.8f) sol.chargeReady=(td.spriteBase==SPR_CAVALRY);
                } else {
                    sol.state=SS_IDLE;
                    sol.chargeMoveTime=0.f;
                }
            }

            sol.angle=lerpAngle(sol.angle,sol.angleTarget,10.f*dt);
        }

        // Update group state based on soldier activity
        bool anyMelee=false,anyMoving=false;
        for(auto& sol:bu.soldiers){
            if(!sol.alive) continue;
            if(sol.inMelee) anyMelee=true;
            if(sol.state==SS_MOVING_SLOT) anyMoving=true;
        }
        if(anyMelee) bu.groupState=UGS_ENGAGED;
        else if(anyMoving) bu.groupState=UGS_ADVANCING;
        else bu.groupState=UGS_IDLE;

        // ═══════════════════════════════════════════════════════════════════════════
        //  ANCHOR UPDATE v8.1: FOLLOW CENTROID OF SOLDIERS
        // ═══════════════════════════════════════════════════════════════════════════
        // Calculate actual centroid of all alive soldiers
        Vector2 actualCentroid={0,0};
        int aliveCnt2=0;
        for(auto& sol:bu.soldiers){
            if(sol.alive){
                actualCentroid=v2add(actualCentroid,sol.pos);
                aliveCnt2++;
            }
        }
        if(aliveCnt2>0) actualCentroid=v2scale(actualCentroid,1.f/(float)aliveCnt2);
        else actualCentroid=bu.anchorPos; // fallback if all dead

        // Move anchor smoothly toward desired position (toward movement target or auto-advance)
        // MINIMAL centroid correction - anchor leads, soldiers follow
        Vector2 targetAnchor=desiredAnchor;

        // v8.3: ULTRA-AGGRESSIVE ADVANCE - almost pure desired movement
        // Only use 10% centroid correction to prevent oscillation
        // This allows anchor to lead units toward enemy without drag
        float centroidWeight=0.1f;  // Minimal correction for all movement types
        targetAnchor.x=targetAnchor.x*(1.f-centroidWeight)+actualCentroid.x*centroidWeight;
        targetAnchor.y=targetAnchor.y*(1.f-centroidWeight)+actualCentroid.y*centroidWeight;

        // Snap to target anchor (v8.3: INSTANT RESPONSE)
        // At 60 FPS: 50.0 * 0.016 = 80% per frame (1-2 frames convergence)
        bu.anchorPos.x+=(targetAnchor.x-bu.anchorPos.x)*50.f*dt;
        bu.anchorPos.y+=(targetAnchor.y-bu.anchorPos.y)*50.f*dt;
    }
}

// Enemy AI: assign orders periodically (3.2: aiReactTimer, differentiated behaviors)
static void updateEnemyAI(float dt){
    int nPlayerAlive=0;
    for(auto& pu:g_battle.playerUnits){
        int ac=0; for(auto& s:pu.soldiers) if(s.alive) ac++;
        if(ac>0) nPlayerAlive++;
    }
    if(nPlayerAlive==0) return;

    for(int ui=0;ui<(int)g_battle.enemyUnits.size();ui++){
        BattleUnit& bu=g_battle.enemyUnits[ui];
        if(bu.groupState==UGS_ROUTING) continue;
        int aliveCnt=0;
        for(auto& sol:bu.soldiers) if(sol.alive) aliveCnt++;
        if(aliveCnt==0) continue;

        // Tick AI react timer
        bu.aiReactTimer-=dt;
        if(bu.aiReactTimer>0) continue; // skip recalculation this frame
        bu.aiReactTimer=1.5f; // recalculate every 1.5s

        if(bu.typeIdx>=unitTypeCount()) continue;
        const UnitTypeDef& td=g_unitTypes[bu.typeIdx];

        // Find nearest player group
        float best=1e9f; int best_u=-1;
        for(int pu=0;pu<(int)g_battle.playerUnits.size();pu++){
            int pAlive=0;
            for(auto& ps:g_battle.playerUnits[pu].soldiers) if(ps.alive) pAlive++;
            if(pAlive==0) continue;
            float d=vdist(bu.anchorPos,g_battle.playerUnits[pu].anchorPos);
            if(d<best){best=d;best_u=pu;}
        }
        if(best_u<0) continue;

        Vector2 targetPos=g_battle.playerUnits[best_u].anchorPos;
        float distToTarget=vdist(bu.anchorPos,targetPos);

        if(td.range>0){
            // 3.2: Ranged — maintain 200px distance, retreat if <150px
            if(distToTarget<150.f){
                // Retreat
                Vector2 away=vnorm(v2sub(bu.anchorPos,targetPos));
                bu.orderTarget=v2add(bu.anchorPos,v2scale(away,200.f));
                bu.orderAttack=-1;
                bu.hasExplicitOrder=true;
                bu.orderType=1; // move
                // FIX: removed bu.anchorPos=bu.orderTarget (was teleporting the unit)
            } else if(distToTarget>220.f){
                bu.orderAttack=best_u;
                bu.orderType=0; // attack
                bu.hasExplicitOrder=false;
            } else {
                // Hold — just attack in range
                bu.orderAttack=best_u;
                bu.orderType=2; // hold
                bu.hasExplicitOrder=false;
            }
        } else if(td.spriteBase==SPR_CAVALRY){
            // 3.2: Cavalry — flanking angle 45-90 degrees lateral
            Vector2 toTarget=vnorm(v2sub(targetPos,bu.anchorPos));
            float flankAngle=DEG2RAD*(45.f+frandMT()*45.f);
            float side=(ui%2==0)?1.f:-1.f;
            Vector2 flankDir={
                toTarget.x*cosf(flankAngle*side)-toTarget.y*sinf(flankAngle*side),
                toTarget.x*sinf(flankAngle*side)+toTarget.y*cosf(flankAngle*side)
            };
            bu.orderTarget=v2add(targetPos,v2scale(flankDir,80.f));
            bu.orderAttack=best_u;
            bu.orderType=3; // flank
            bu.hasExplicitOrder=true;
            bu.anchorPos=v2add(bu.anchorPos,v2scale(vnorm(v2sub(bu.orderTarget,bu.anchorPos)),5.f));
        } else {
            // 3.2: Infantry — distribute across different player groups if multiple exist
            if(nPlayerAlive>1){
                // Spread: pick target based on our index
                int targetIdx=ui % nPlayerAlive;
                int counted=0;
                for(int pu=0;pu<(int)g_battle.playerUnits.size();pu++){
                    int ac=0; for(auto& ps:g_battle.playerUnits[pu].soldiers) if(ps.alive) ac++;
                    if(ac>0){
                        if(counted==targetIdx){best_u=pu;break;}
                        counted++;
                    }
                }
            }
            bu.orderAttack=best_u;
            bu.orderType=0; // attack
            bu.hasExplicitOrder=false;
        }

        // Update formation slots toward target (6.6: use proper facing angle)
        if(bu.orderAttack>=0&&validIdx(bu.orderAttack,g_battle.playerUnits)){
            auto slots=calcFormationSlots(bu.anchorPos,(int)bu.soldiers.size(),bu.formationFacing);
            for(int s=0;s<(int)bu.soldiers.size()&&s<(int)slots.size();s++)
                bu.soldiers[s].formationSlot=slots[s];
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  BATTLE DRAW
// ═══════════════════════════════════════════════════════════════════════════
static void drawBattlefield(){
    // Terrain background
    ClearBackground(C_TERRAIN);
    // Variation noise (static, initialized once)
    static bool terrainInit=false;
    static Image terrainNoise;
    static Texture2D terrainTex={0};
    if(!terrainInit){
        terrainNoise=GenImageColor(BATTLE_W,BATTLE_H,C_TERRAIN);
        srand(42);
        for(int y2=0;y2<BATTLE_H;y2+=8){
            for(int x2=0;x2<BATTLE_W;x2+=8){
                int v=rand()%12-6;
                Color c2={clampU8((int)C_TERRAIN.r+v),clampU8((int)C_TERRAIN.g+v),clampU8((int)C_TERRAIN.b+v),255};
                ImageDrawRectangle(&terrainNoise,x2,y2,8,8,c2);
            }
        }
        terrainTex=LoadTextureFromImage(terrainNoise);
        UnloadImage(terrainNoise);
        terrainInit=true; // 1.1: was missing, caused re-generation every frame
    }
    DrawTexture(terrainTex,0,0,WHITE);

    // Grid
    for(int x2=0;x2<BATTLE_W;x2+=128)
        DrawLine(x2,0,x2,BATTLE_H,{35,46,26,80});
    for(int y2=0;y2<BATTLE_H;y2+=128)
        DrawLine(0,y2,BATTLE_W,y2,{35,46,26,80});

    // Center line
    DrawLine(BATTLE_W/2,0,BATTLE_W/2,BATTLE_H,{50,70,35,100});

    // Obstacles
    for(auto& obs:g_battle.obstacles){
        Color oc;
        if(g_battle.terrain==TERRAIN_FOREST) oc={25,55,20,200};
        else oc={90,80,70,200};
        DrawRectangleRec(obs,oc);
        DrawRectangleLinesEx(obs,2,{20,45,15,255});
    }

    // Deployment zones (faint)
    DrawRectangle(0,0,(int)(BATTLE_W*0.35f),BATTLE_H,{80,120,220,12});
    DrawRectangle((int)(BATTLE_W*0.65f),0,(int)(BATTLE_W*0.35f),BATTLE_H,{220,60,50,12});
}

static void drawAllUnits(){
    // Dead markers
    for(auto& d:g_battle.dead){
        int a=(int)(d.alpha*160);
        Color dc={60,20,20,(unsigned char)a};
        if(d.isCavalry){
            DrawEllipse((int)d.pos.x,(int)d.pos.y,12,8,dc);
        } else {
            DrawCircleV(d.pos,7,dc);
        }
        DrawLine((int)(d.pos.x-8),(int)(d.pos.y-8),(int)(d.pos.x+8),(int)(d.pos.y+8),{80,20,20,(unsigned char)a});
        DrawLine((int)(d.pos.x+8),(int)(d.pos.y-8),(int)(d.pos.x-8),(int)(d.pos.y+8),{80,20,20,(unsigned char)a});
    }

    // Projectiles
    for(auto& p:g_battle.projs){
        if(!p.alive) continue;
        Color pc=p.isCrossbow?Color{150,140,100,255}:Color{180,140,60,255};
        if(p.fromPlayer) pc={pc.r,(unsigned char)(pc.g+20),pc.b,255};
        DrawCircleV(p.pos,3,pc);
    }

    // Draw order lines for selected player units
    for(auto& bu:g_battle.playerUnits){
        if(!bu.selected) continue;
        // Show current orders visually
        if(bu.orderAttack>=0&&bu.orderAttack<(int)g_battle.enemyUnits.size()){
            // Attack order — red line to target
            DrawLineEx(bu.anchorPos,g_battle.enemyUnits[bu.orderAttack].anchorPos,2.5f,{255,80,80,160});
            // Arrow marker on enemy
            Vector2 dir=vnorm(v2sub(g_battle.enemyUnits[bu.orderAttack].anchorPos,bu.anchorPos));
            Vector2 markerPos=v2sub(g_battle.enemyUnits[bu.orderAttack].anchorPos,v2scale(dir,30.f));
            DrawCircleV(markerPos,8,{255,50,50,200});
            DrawText("⚔",(int)(markerPos.x-8),(int)(markerPos.y-8),16,{255,100,100,255});
        } else if(bu.hasExplicitOrder&&bu.orderType==1){
            // Move order — blue line to destination
            DrawLineEx(bu.anchorPos,bu.orderTarget,2.5f,{100,180,255,160});
            // Target marker
            DrawCircleLines((int)bu.orderTarget.x,(int)bu.orderTarget.y,12,{100,180,255,220});
            DrawText("→",(int)(bu.orderTarget.x-8),(int)(bu.orderTarget.y-6),16,{100,200,255,255});
        }
    }

    // Draw enemy units
    for(auto& bu:g_battle.enemyUnits){
        if(bu.typeIdx>=unitTypeCount()) continue;
        const UnitTypeDef& td=g_unitTypes[bu.typeIdx];
        int alive=0; for(auto& s:bu.soldiers) if(s.alive) alive++;
        if(alive==0) continue;
        // Group bounding circle
        DrawCircleLines((int)bu.anchorPos.x,(int)bu.anchorPos.y,
                        (int)(soldierMeleeRadius(td)*(float)alive*0.4f+20.f),
                        {180,40,40,40});
        for(auto& sol:bu.soldiers){
            if(!sol.alive) continue;
            drawSoldierSprite(g_enemyTextures[bu.typeIdx],sol.pos,sol.angle,0.65f,WHITE,true);
            // 3.5: HP bar only when damaged
            if(sol.hp<td.hpPerSoldier){
                float bw=8.f; float ratio=sol.hp/td.hpPerSoldier;
                DrawRectangle((int)(sol.pos.x-bw/2),(int)(sol.pos.y-14),(int)bw,2,DARKGRAY);
                Color hc=ratio>0.5f?Color{0,228,48,255}:ratio>0.25f?Color{253,249,0,255}:Color{230,41,55,255};
                DrawRectangle((int)(sol.pos.x-bw/2),(int)(sol.pos.y-14),(int)(bw*ratio),2,hc);
            }
        }
        // Morale bar
        float mr=bu.morale/100.f;
        DrawRectangle((int)(bu.anchorPos.x-30),(int)(bu.anchorPos.y-28),60,5,DARKGRAY);
        DrawRectangle((int)(bu.anchorPos.x-30),(int)(bu.anchorPos.y-28),(int)(60*mr),5,
                      mr>0.5f?Color{255,161,0,255}:mr>0.2f?Color{253,249,0,255}:Color{230,41,55,255});
    }

    // Draw player units
    for(auto& bu:g_battle.playerUnits){
        if(bu.typeIdx>=unitTypeCount()) continue;
        const UnitTypeDef& td=g_unitTypes[bu.typeIdx];
        int alive=0; for(auto& s:bu.soldiers) if(s.alive) alive++;
        if(alive==0) continue;
        if(bu.selected){
            // 3.10: Pulsing selection circle
            float pulse=soldierMeleeRadius(td)*(float)alive*0.4f+24.f+sinf(g_menuTime*4.f)*4.f;
            DrawCircleLines((int)bu.anchorPos.x,(int)bu.anchorPos.y,
                            (int)pulse,
                            {C_ALLY.r,C_ALLY.g,C_ALLY.b,150});
            // 3.10: Soldier count above anchor
            char cntbuf[16]; snprintf(cntbuf,15,"%d",alive);
            int ctw=MeasureText(cntbuf,11);
            DrawText(cntbuf,(int)(bu.anchorPos.x-ctw/2),(int)(bu.anchorPos.y-pulse-14),11,C_ALLY);
        }
        for(auto& sol:bu.soldiers){
            if(!sol.alive) continue;
            Color tint={td.r,td.g,td.b,255};
            if(bu.selected) tint=WHITE;
            drawSoldierSprite(g_playerTextures[bu.typeIdx],sol.pos,sol.angle,0.65f,tint,true);
            // 3.5: HP bar only when damaged
            if(sol.hp<td.hpPerSoldier){
                float bw=8.f; float ratio=sol.hp/td.hpPerSoldier;
                DrawRectangle((int)(sol.pos.x-bw/2),(int)(sol.pos.y-14),(int)bw,2,DARKGRAY);
                Color hc=ratio>0.5f?Color{0,228,48,255}:ratio>0.25f?Color{253,249,0,255}:Color{230,41,55,255};
                DrawRectangle((int)(sol.pos.x-bw/2),(int)(sol.pos.y-14),(int)(bw*ratio),2,hc);
            }
        }
        // 6.2: Veterancy stars above anchor
        if(bu.veterancy>0){
            for(int v=0;v<bu.veterancy;v++){
                DrawText("★",(int)(bu.anchorPos.x-10+v*10),(int)(bu.anchorPos.y-36),12,C_GOLD);
            }
        }
        // Morale bar
        float mr=bu.morale/100.f;
        DrawRectangle((int)(bu.anchorPos.x-30),(int)(bu.anchorPos.y-28),60,5,DARKGRAY);
        DrawRectangle((int)(bu.anchorPos.x-30),(int)(bu.anchorPos.y-28),(int)(60*mr),5,
                      mr>0.5f?Color{0,158,47,255}:mr>0.2f?Color{253,249,0,255}:Color{230,41,55,255});
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  HUD DRAW (battle)
// ═══════════════════════════════════════════════════════════════════════════
static void drawBattleHUD(Vector2 mouse){
    int hudY=SCREEN_H-HUD_H;

    // Top bar
    DrawRectangle(0,0,SCREEN_W,TOPBAR_H,{0,0,0,220});
    DrawText(g_battle.scenarioName,10,8,14,C_GOLD);
    DrawText(TextFormat("Turn: %d  |  [SPACE] Speed  |  [ESC] Pause  |  [LClick+Drag] Select  |  [RClick] Give Orders",
             g_campaign.turn),
             SCREEN_W/2-320,8,10,C_SECONDARY);
    DrawText("Right-click unit/location to command — Left sidebar shows orders",
             SCREEN_W/2-280, 20, 9, Color{180, 160, 120, 200});

    // Bottom HUD
    DrawRectangle(0,hudY,SCREEN_W,HUD_H,{8,6,4,240});
    DrawLine(0,hudY,SCREEN_W,hudY,{80,65,30,200});

    // Selected unit portrait (left)
    BattleUnit* sel=nullptr;
    for(auto& bu:g_battle.playerUnits) if(bu.selected&&bu.typeIdx<unitTypeCount()){sel=&bu;break;}

    if(sel){
        const UnitTypeDef& td=g_unitTypes[sel->typeIdx];
        int alive=0; for(auto& s:sel->soldiers) if(s.alive) alive++;
        // Portrait box
        DrawRectangle(8,hudY+6,58,58,{td.r,td.g,td.b,100});
        DrawRectangleLinesEx({8,(float)(hudY+6),58,58},2,C_GOLD);
        drawSoldierSprite(g_playerTextures[sel->typeIdx],{37.f,(float)(hudY+35)},
                          g_menuTime*30.f,1.4f,{td.r,td.g,td.b,255},false);
        // Info
        DrawText(td.name,72,hudY+8,15,C_PARCHMENT);
        // HP bar
        float total=(float)(td.soldierCount)*td.hpPerSoldier;
        float curHp=0.f; for(auto& s:sel->soldiers) if(s.alive) curHp+=s.hp;
        float hpRat=curHp/total;
        DrawRectangle(72,hudY+28,220,10,DARKGRAY);
        DrawRectangle(72,hudY+28,(int)(220*hpRat),10,hpRat>0.5f?Color{0,228,48,255}:hpRat>0.25f?Color{253,249,0,255}:Color{230,41,55,255});
        // Morale bar
        DrawRectangle(72,hudY+42,220,8,DARKGRAY);
        float mr=sel->morale/100.f;
        DrawRectangle(72,hudY+42,(int)(220*mr),8,mr>0.5f?Color{0,158,47,255}:mr>0.2f?Color{253,249,0,255}:Color{230,41,55,255});
        DrawText(TextFormat("Morale: %.0f%%",sel->morale),298,hudY+41,12,C_SECONDARY);
        DrawText(TextFormat("Soldiers: %d / %d",alive,td.soldierCount),72,hudY+54,12,C_SECONDARY);
    } else {
        DrawText("No unit selected",12,hudY+34,14,{70,70,70,255});
    }

    // Center: group state and current orders
    if(sel){
        static const char* gsNames[]={"Idle","Advancing","Engaged","Routing"};
        const char* gsn=gsNames[(int)sel->groupState];
        Color gsc=sel->groupState==UGS_ENGAGED?C_ENEMY_COL:
                  sel->groupState==UGS_ROUTING?Color{230,41,55,255}:
                  sel->groupState==UGS_ADVANCING?C_ALLY:C_SECONDARY;
        int tw=MeasureText(gsn,16);
        DrawText(gsn,SCREEN_W/2-tw/2,hudY+25,16,gsc);

        // Show current orders
        if(sel->hasExplicitOrder){
            if(sel->orderAttack>=0&&sel->orderAttack<(int)g_battle.enemyUnits.size()){
                DrawText("📋 ATTACKING ENEMY UNIT",SCREEN_W/2-115,hudY+48,11,{255,100,100,255});
            } else if(sel->orderType==1){
                DrawText("📍 MOVING TO POSITION",SCREEN_W/2-110,hudY+48,11,{100,180,255,255});
            }
        } else {
            DrawText("🎯 IDLE (AUTO)",SCREEN_W/2-60,hudY+48,11,C_SECONDARY);
        }
    }

    // Right: counters + pause button
    int pa=0,ea=0;
    for(auto& bu:g_battle.playerUnits) for(auto& s:bu.soldiers) if(s.alive) pa++;
    for(auto& bu:g_battle.enemyUnits) for(auto& s:bu.soldiers) if(s.alive) ea++;
    DrawText(TextFormat("Allies: %d",pa),(int)(SCREEN_W-320),hudY+14,14,{80,160,255,255});
    DrawText(TextFormat("Enemies: %d",ea),(int)(SCREEN_W-320),hudY+34,14,C_ENEMY_COL);

    if(drawSmBtn({(float)(SCREEN_W-120),(float)(hudY+8),110,36},"PAUSE",mouse,{40,35,20,255},{70,60,35,255}))
        g_battle.paused=true;
    if(drawSmBtn({(float)(SCREEN_W-120),(float)(hudY+50),110,28},
                 g_battle.timeScale>1.f?"Speed: x2":"Speed: x1",mouse,
                 {30,30,50,255},{50,50,80,255}))
        g_battle.timeScale=(g_battle.timeScale>1.f)?1.f:2.f;

    // 3.6: Minimap (150x90px, bottom-right above HUD)
    int mmW=150,mmH=90;
    int mmX=SCREEN_W-160, mmY=hudY-mmH-8;
    DrawRectangle(mmX,mmY,mmW,mmH,{0,0,0,180});
    DrawRectangleLinesEx({(float)mmX,(float)mmY,(float)mmW,(float)mmH},1,{80,65,30,200});
    // Draw unit dots
    for(auto& bu:g_battle.playerUnits){
        int ac=0; for(auto& s:bu.soldiers) if(s.alive) ac++;
        if(ac==0) continue;
        int mx2=(int)(mmX+bu.anchorPos.x/(float)BATTLE_W*mmW);
        int my2=(int)(mmY+bu.anchorPos.y/(float)BATTLE_H*mmH);
        DrawRectangle(mx2-2,my2-2,4,4,C_ALLY);
    }
    for(auto& bu:g_battle.enemyUnits){
        int ac=0; for(auto& s:bu.soldiers) if(s.alive) ac++;
        if(ac==0) continue;
        int mx2=(int)(mmX+bu.anchorPos.x/(float)BATTLE_W*mmW);
        int my2=(int)(mmY+bu.anchorPos.y/(float)BATTLE_H*mmH);
        DrawRectangle(mx2-2,my2-2,4,4,C_ENEMY_COL);
    }
    // Draw viewport rect on minimap
    float vzl=g_battle.cam.zoom;
    float vwW=(float)SCREEN_W/(vzl*(float)BATTLE_W)*mmW;
    float vwH=(float)(SCREEN_H-HUD_H)/(vzl*(float)BATTLE_H)*mmH;
    float vwX=mmX+(g_battle.cam.target.x-(float)SCREEN_W*0.5f/vzl)/(float)BATTLE_W*mmW;
    float vwY=mmY+(g_battle.cam.target.y-(float)(SCREEN_H-HUD_H)*0.5f/vzl)/(float)BATTLE_H*mmH;
    DrawRectangleLinesEx({vwX,vwY,vwW,vwH},1,WHITE);

    // 6.4: Battle log (last 5 entries, fade older ones)
    int logX=8, logY=hudY-8;
    int logCount=std::min(5,(int)g_battleLog.size());
    for(int i=0;i<logCount;i++){
        unsigned char a=(unsigned char)(200-i*35);
        DrawText(g_battleLog[i].c_str(),logX,logY-(i+1)*16,10,{180,160,120,a});
    }

    // 4.9: FPS counter
    if(g_settings.showFPS) DrawFPS(8,8);
}

// ═══════════════════════════════════════════════════════════════════════════
//  STATE: BATTLE
// ═══════════════════════════════════════════════════════════════════════════
static GameState updateDrawBattle(Vector2 mouse,float dt){
    float eff=dt*g_battle.timeScale;

    // 4.8: Camera target and zoom targets for smooth lerp
    static Vector2 g_camTarget={BATTLE_W/2.f,BATTLE_H/2.f};
    static float   g_zoomTarget=1.f;

    // Controls
    if(IsKeyPressed(KEY_ESCAPE)) g_battle.paused=!g_battle.paused;
    if(IsKeyPressed(KEY_SPACE)) g_battle.timeScale=(g_battle.timeScale>1.f)?1.f:2.f;

    // Camera pan (arrows / WASD)
    float panSpd=400.f/g_battle.cam.zoom;
    if(IsKeyDown(KEY_RIGHT)||IsKeyDown(KEY_D)) g_camTarget.x+=panSpd*dt;
    if(IsKeyDown(KEY_LEFT)||IsKeyDown(KEY_A))  g_camTarget.x-=panSpd*dt;
    if(IsKeyDown(KEY_DOWN)||IsKeyDown(KEY_S))  g_camTarget.y+=panSpd*dt;
    if(IsKeyDown(KEY_UP)||IsKeyDown(KEY_W))    g_camTarget.y-=panSpd*dt;
    // Zoom
    float wheel=GetMouseWheelMove();
    if(wheel!=0){
        g_zoomTarget+=wheel*0.1f;
        g_zoomTarget=std::max(0.5f,std::min(2.f,g_zoomTarget));
    }
    // Clamp target
    float hw=(SCREEN_W*0.5f)/g_battle.cam.zoom;
    float hh=((SCREEN_H-HUD_H-TOPBAR_H)*0.5f)/g_battle.cam.zoom;
    g_camTarget.x=std::max(hw,std::min((float)BATTLE_W-hw,g_camTarget.x));
    g_camTarget.y=std::max(hh,std::min((float)BATTLE_H-hh,g_camTarget.y));
    // 4.8: Smooth lerp
    g_battle.cam.target.x+=(g_camTarget.x-g_battle.cam.target.x)*8.f*dt;
    g_battle.cam.target.y+=(g_camTarget.y-g_battle.cam.target.y)*8.f*dt;
    g_battle.camZoom+=(g_zoomTarget-g_battle.camZoom)*6.f*dt;
    g_battle.cam.zoom=g_battle.camZoom;
    g_battle.cam.offset={(float)SCREEN_W*0.5f,(float)(SCREEN_H-HUD_H+TOPBAR_H)*0.5f};

    // Pause overlay
    if(g_battle.paused){
        // Still draw world
        BeginMode2D(g_battle.cam);
        drawBattlefield();
        drawAllUnits();
        EndMode2D();
        drawBattleHUD(mouse);
        // Overlay
        DrawRectangle(0,0,SCREEN_W,SCREEN_H,{0,0,0,160});
        const char* pm="PAUSED";
        int ptw=MeasureText(pm,52);
        DrawText(pm,SCREEN_W/2-ptw/2,SCREEN_H/2-80,52,C_GOLD);
        if(drawButton({(float)(SCREEN_W/2-130),(float)(SCREEN_H/2+10),260,50},"RESUME",mouse))
            g_battle.paused=false;
        if(drawButton({(float)(SCREEN_W/2-130),(float)(SCREEN_H/2+70),260,50},"ABANDON BATTLE",mouse,
                       {60,20,20,255},{100,40,40,255})){
            g_battle.paused=false;
            g_battle.battleOver=true;
            g_battle.playerWon=false;
        }
        return STATE_BATTLE;
    }

    if(!g_battle.battleOver){
        // Selection
        bool overHud=(mouse.y>SCREEN_H-HUD_H||mouse.y<TOPBAR_H);
        if(!overHud){
            // Convert mouse to world coords
            Vector2 worldMouse=GetScreenToWorld2D(mouse,g_battle.cam);

            if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                g_battle.selStart=worldMouse;
                g_battle.dragging=true;
                for(auto& bu:g_battle.playerUnits) bu.selected=false;
            }
            if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)&&g_battle.dragging){
                g_battle.selRect.x=fminf(worldMouse.x,g_battle.selStart.x);
                g_battle.selRect.y=fminf(worldMouse.y,g_battle.selStart.y);
                g_battle.selRect.width=fabsf(worldMouse.x-g_battle.selStart.x);
                g_battle.selRect.height=fabsf(worldMouse.y-g_battle.selStart.y);
            }
            if(IsMouseButtonReleased(MOUSE_LEFT_BUTTON)){
                g_battle.dragging=false;
                if(g_battle.selRect.width<8&&g_battle.selRect.height<8){
                    // Single click
                    float best=9999; int bidx=-1;
                    for(int i=0;i<(int)g_battle.playerUnits.size();i++){
                        float d=vdist(worldMouse,g_battle.playerUnits[i].anchorPos);
                        if(d<best&&d<40){best=d;bidx=i;}
                    }
                    if(bidx>=0) g_battle.playerUnits[bidx].selected=true;
                } else {
                    for(auto& bu:g_battle.playerUnits){
                        if(ptInRect(bu.anchorPos,g_battle.selRect)) bu.selected=true;
                    }
                }
                g_battle.selRect={};
            }
            if(IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)){
                // Check for enemy target (v7.3: smaller radius to avoid confusion between move/attack)
                int eIdx=-1; float bestD=9999;
                for(int i=0;i<(int)g_battle.enemyUnits.size();i++){
                    float d=vdist(worldMouse,g_battle.enemyUnits[i].anchorPos);
                    if(d<bestD&&d<40){bestD=d;eIdx=i;}  // Reduced from 60 to 40 for clearer distinction
                }
                int offX=0;
                for(auto& bu:g_battle.playerUnits){
                    if(!bu.selected) continue;
                    if(eIdx>=0){
                        // Attack order — target specific enemy unit (v7.3: direct target only)
                        bu.orderAttack=eIdx;
                        bu.hasExplicitOrder=true;
                        bu.orderType=0; // attack
                        bu.groupState=UGS_ADVANCING;
                        // Clear movement target
                        bu.orderTarget=bu.anchorPos;
                        // Reset soldier targeting to force refresh
                        for(auto& sol:bu.soldiers){
                            sol.attackTargetUnit=-1;
                            sol.attackTargetSoldier=-1;
                            sol.targetRefreshTimer=0.f;
                        }
                    } else {
                        // Movement order (v7.3: click on empty space to move)
                        bu.orderAttack=-1;
                        bu.hasExplicitOrder=true;
                        bu.orderType=1; // move
                        bu.orderTarget={worldMouse.x+(float)offX,worldMouse.y};
                        bu.groupState=UGS_ADVANCING;
                        // FIX: update formationFacing immediately so slots recalc points the right way
                        Vector2 moveDir=vnorm(v2sub(bu.orderTarget,bu.anchorPos));
                        bu.formationFacing=dirToAngle(moveDir);
                        // Recalculate slots now with the new facing and current anchor
                        {
                            auto slots=calcFormationSlots(bu.anchorPos,(int)bu.soldiers.size(),bu.formationFacing);
                            for(int s=0;s<(int)bu.soldiers.size()&&s<(int)slots.size();s++)
                                bu.soldiers[s].formationSlot=slots[s];
                        }
                        // Reset soldier targeting — they follow formation, not auto-target during movement
                        for(auto& sol:bu.soldiers){
                            sol.attackTargetUnit=-1;
                            sol.attackTargetSoldier=-1;
                            sol.targetRefreshTimer=0.f;
                            sol.state=SS_MOVING_TARGET;
                        }
                        offX=(offX>0)?(-offX):((-offX)+50);
                    }
                }
            }
            // 6.6: Shift+RClick to set formation facing angle
            if(IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)&&IsKeyDown(KEY_LEFT_SHIFT)){
                for(auto& bu:g_battle.playerUnits){
                    if(!bu.selected) continue;
                    Vector2 dir=vnorm(v2sub(worldMouse,bu.anchorPos));
                    bu.formationFacing=dirToAngle(dir);
                    // Recalc formation slots with new facing
                    auto slots=calcFormationSlots(bu.anchorPos,(int)bu.soldiers.size(),bu.formationFacing);
                    for(int s=0;s<(int)bu.soldiers.size()&&s<(int)slots.size();s++)
                        bu.soldiers[s].formationSlot=slots[s];
                }
            }
        }

        // Update
        updateEnemyAI(eff);
        updateBattleUnits(g_battle.playerUnits,g_battle.enemyUnits,eff);
        updateBattleUnits(g_battle.enemyUnits,g_battle.playerUnits,eff);
        separateSoldiers(g_battle.playerUnits);
        separateSoldiers(g_battle.enemyUnits);

        // Update projectiles
        for(auto& p:g_battle.projs){
            if(!p.alive) continue;
            p.pos=v2add(p.pos,v2scale(p.vel,eff));
            if(p.pos.x<0||p.pos.x>BATTLE_W||p.pos.y<0||p.pos.y>BATTLE_H){p.alive=false;continue;}
            auto& targets=p.fromPlayer?g_battle.enemyUnits:g_battle.playerUnits;
            for(auto& bu:targets){
                if(!p.alive) break;
                for(auto& sol:bu.soldiers){
                    if(!sol.alive) continue;
                    if(vdist(p.pos,sol.pos)<14.f){
                        sol.hp-=p.dmg;
                        p.alive=false;
                        if(sol.hp<=0){
                            sol.alive=false;
                            bool isCav=g_unitTypes[bu.typeIdx].spriteBase==SPR_CAVALRY;
                            g_battle.dead.push_back({sol.pos,1.f,8.f,isCav});
                        }
                        break;
                    }
                }
            }
        }
        // 2.5: Projectile cleanup with swap-with-back (faster than erase)
        for(int pi=(int)g_battle.projs.size()-1;pi>=0;pi--){
            if(!g_battle.projs[pi].alive){
                g_battle.projs[pi]=g_battle.projs.back();
                g_battle.projs.pop_back();
            }
        }

        // Fade dead markers (2.5: swap-with-back)
        for(auto& d:g_battle.dead){d.timer-=eff;d.alpha=d.timer/8.f;}
        for(int di=(int)g_battle.dead.size()-1;di>=0;di--){
            if(g_battle.dead[di].timer<=0){
                g_battle.dead[di]=g_battle.dead.back();
                g_battle.dead.pop_back();
            }
        }

        // Check win / lose
        int pa=0,ea=0;
        for(auto& bu:g_battle.playerUnits) for(auto& s:bu.soldiers) if(s.alive) pa++;
        for(auto& bu:g_battle.enemyUnits) for(auto& s:bu.soldiers) if(s.alive) ea++;
        if(pa==0){g_battle.battleOver=true;g_battle.playerWon=false;
            battleLogAdd("DEFEAT — all forces destroyed!");}
        if(ea==0&&pa>0){g_battle.battleOver=true;g_battle.playerWon=true;
            battleLogAdd("VICTORY — all enemies routed!");
            // 6.2: Grant veterancy to survivors with >5 kills
            for(auto& bu:g_battle.playerUnits){
                int topKills=0;
                for(auto& s:bu.soldiers) if(s.alive) topKills=std::max(topKills,s.kills);
                if(topKills>5&&bu.veterancy<3){
                    bu.veterancy++;
                    char vbuf[128];
                    snprintf(vbuf,127,"[T%d] %s promoted to level %d!",
                        g_campaign.turn,g_unitTypes[bu.typeIdx].name,bu.veterancy);
                    battleLogAdd(vbuf);
                }
            }
        }
    }

    // DRAW
    BeginMode2D(g_battle.cam);
    drawBattlefield();
    // Selection rect in world space
    if(g_battle.dragging&&(g_battle.selRect.width>8||g_battle.selRect.height>8)){
        DrawRectangleRec(g_battle.selRect,{100,180,255,25});
        DrawRectangleLinesEx(g_battle.selRect,1,C_ALLY);
    }
    drawAllUnits();
    EndMode2D();

    drawBattleHUD(mouse);

    // Game over overlay
    if(g_battle.battleOver){
        g_battle.resultTimer+=dt;
        float fade=std::min(1.f,g_battle.resultTimer*0.8f);
        Color overlay=g_battle.playerWon?Color{0,80,0,(unsigned char)(int)(160*fade)}:
                                          Color{80,0,0,(unsigned char)(int)(160*fade)};
        DrawRectangle(0,0,SCREEN_W,SCREEN_H,overlay);
        if(fade>0.5f){
            const char* w=g_battle.playerWon?"VICTORY!":"DEFEAT!";
            int tsz=64;
            int tw2=MeasureText(w,tsz);
            Color tc=g_battle.playerWon?Color{0,228,48,255}:Color{230,41,55,255};
            DrawText(w,SCREEN_W/2-tw2/2+3,SCREEN_H/2-70+3,tsz,{0,0,0,200});
            DrawText(w,SCREEN_W/2-tw2/2,SCREEN_H/2-70,tsz,tc);
            if(drawButton({(float)(SCREEN_W/2-160),(float)(SCREEN_H/2+20),320,54},"VIEW RESULTS",mouse)){
                // Build result
                g_lastResult.playerWon=g_battle.playerWon;
                g_lastResult.playerLosses=0; g_lastResult.enemyLosses=0;
                g_lastResult.survivors.clear();
                g_lastResult.lootApplied=false; // 0.2: reset before entering result screen
                for(auto& bu:g_battle.playerUnits){
                    int alive=0; for(auto& s:bu.soldiers) if(s.alive) alive++;
                    int dead=0;  for(auto& s:bu.soldiers) if(!s.alive) dead++;
                    g_lastResult.playerLosses+=dead;
                    g_lastResult.survivors.push_back({bu.typeIdx,alive});
                }
                for(auto& bu:g_battle.enemyUnits){
                    for(auto& s:bu.soldiers) if(!s.alive) g_lastResult.enemyLosses++;
                }
                g_lastResult.lootGold=g_battle.playerWon&&!g_battle.isDefense?
                    (50.f+frandMT()*100.f):0.f;
                g_lastResult.provinceIdx=g_battle.battleProvince;
                g_lastResult.isDefense=g_battle.isDefense;
                return STATE_BATTLE_RESULT;
            }
        }
    }

    return STATE_BATTLE;
}

// ═══════════════════════════════════════════════════════════════════════════
//  STATE: BATTLE RESULT
// ═══════════════════════════════════════════════════════════════════════════
static GameState updateDrawBattleResult(Vector2 mouse){
    ClearBackground({8,6,4,255});
    // Gradient overlay — single GPU call instead of per-line loop (2.1)
    Color topCol=g_lastResult.playerWon?Color{0,30,10,255}:Color{30,0,0,255};
    DrawRectangleGradientV(0,0,SCREEN_W,SCREEN_H,topCol,C_BG);

    // Title
    const char* ttl=g_lastResult.playerWon?"VICTORY":"DEFEAT";
    int tsz=60;
    int ttw=MeasureText(ttl,tsz);
    Color tc=g_lastResult.playerWon?Color{80,220,80,255}:Color{220,60,60,255};
    DrawText(ttl,SCREEN_W/2-ttw/2+3,40+3,tsz,{0,0,0,160});
    DrawText(ttl,SCREEN_W/2-ttw/2,40,tsz,tc);
    DrawLine(SCREEN_W/2-200,110,SCREEN_W/2+200,110,C_GOLD);

    // Stats
    DrawText(TextFormat("Enemy soldiers killed: %d",g_lastResult.enemyLosses),
             SCREEN_W/2-220,130,16,C_PARCHMENT);
    DrawText(TextFormat("Friendly losses: %d",g_lastResult.playerLosses),
             SCREEN_W/2-220,156,16,C_PARCHMENT);
    if(g_lastResult.lootGold>0){
        DrawText(TextFormat("Loot collected: %.0f gold",g_lastResult.lootGold),
                 SCREEN_W/2-220,182,16,C_GOLD);
    }

    // Survivor list
    DrawText("Surviving units:",SCREEN_W/2-220,220,15,C_GOLD);
    for(int i=0;i<(int)g_lastResult.survivors.size();i++){
        auto [ti,cnt]=g_lastResult.survivors[i];
        if(ti>=unitTypeCount()) continue;
        const UnitTypeDef& td=g_unitTypes[ti];
        Color col={td.r,td.g,td.b,255};
        DrawRectangle(SCREEN_W/2-220,240+i*24,18,18,col);
        DrawText(TextFormat("%s  —  %d soldiers surviving",td.name,cnt),
                 SCREEN_W/2-196,242+i*24,14,C_PARCHMENT);
    }

    // Province change
    if(g_lastResult.playerWon&&!g_lastResult.isDefense&&g_lastResult.provinceIdx>=0
       &&g_lastResult.provinceIdx<(int)g_campaign.provinces.size()){
        Province& prov=g_campaign.provinces[g_lastResult.provinceIdx];
        DrawText(TextFormat("Province captured: %s",prov.name),
                 SCREEN_W/2-220,240+(int)g_lastResult.survivors.size()*24+20,16,C_GOLD);
        // Actually update ownership
        if(prov.owner!=FACTION_PLAYER){
            prov.owner=FACTION_PLAYER;
            g_campaign.playerProvince=g_lastResult.provinceIdx;
        }
    }

    // Apply loot to campaign (0.2: was static bool, now per-result field)
    if(!g_lastResult.lootApplied){
        g_campaign.res.gold+=g_lastResult.lootGold;
        g_lastResult.lootApplied=true;
        // Update player army with survivors (so dead soldiers don't "resurrect")
        if(!g_quickBattle){
            g_campaign.playerArmy.clear();
            for(int i=0;i<(int)g_lastResult.survivors.size();i++){
                int ti=g_lastResult.survivors[i].first;
                int cnt=g_lastResult.survivors[i].second;
                if(ti>=0&&ti<unitTypeCount()&&cnt>0)
                    g_campaign.playerArmy.push_back({ti,cnt});
            }
            g_campaign.readyUnits=g_campaign.playerArmy;
            // Track stats for 3.4
            if(g_lastResult.playerWon) g_campaign.battlesWon++;
            else g_campaign.battlesLost++;
        }
    }

    // (Survivor list and province capture already shown above; army updated in lootApplied block)

    // Continue button
    if(drawButton({(float)(SCREEN_W/2-140),(float)(SCREEN_H-80),280,54},"CONTINUE",mouse)){
        g_lastResult.lootApplied=false;  // reset for next battle
        if(g_quickBattle){
            g_quickBattle=false;
            return STATE_MAIN_MENU;
        }
        return STATE_CAMPAIGN_MAP;
    }
    return STATE_BATTLE_RESULT;
}

// ═══════════════════════════════════════════════════════════════════════════
//  STATE: PRE-BATTLE
// ═══════════════════════════════════════════════════════════════════════════
static GameState updateDrawPreBattle(Vector2 mouse){
    ClearBackground(C_BG);
    for(int gx=0;gx<SCREEN_W;gx+=40) DrawLine(gx,0,gx,SCREEN_H,{20,18,14,50});
    for(int gy=0;gy<SCREEN_H;gy+=40) DrawLine(0,gy,SCREEN_W,gy,{20,18,14,50});

    DrawRectangle(0,0,SCREEN_W,50,{0,0,0,220});
    DrawText("PRE-BATTLE DEPLOYMENT",14,10,26,C_GOLD);
    if(g_preBattle.provinceIdx>=0&&g_preBattle.provinceIdx<(int)g_campaign.provinces.size())
        DrawText(g_campaign.provinces[g_preBattle.provinceIdx].name,14,38,12,C_SECONDARY);

    float halfH=(SCREEN_H-50)/2.f;
    // Top half: player units
    DrawRectangle(0,50,SCREEN_W,(int)halfH,{8,12,20,200});
    DrawText("YOUR FORCES — Select units to deploy (max 12 groups):",14,58,14,C_ALLY);
    DrawLine(0,50+halfH,(int)SCREEN_W,50+(int)halfH,{80,65,30,120});

    int deployCount=0;
    for(bool b:g_preBattle.include) if(b) deployCount++;

    // Ensure include vector sized
    g_preBattle.include.resize(g_campaign.readyUnits.size(),false);

    for(int i=0;i<(int)g_campaign.readyUnits.size();i++){
        auto [ti,cnt]=g_campaign.readyUnits[i];
        if(ti>=unitTypeCount()) continue;
        const UnitTypeDef& td=g_unitTypes[ti];
        float ry=80.f+i*36.f;
        if(ry+36>50+halfH) break;
        Color rowbg=(i%2==0)?Color{10,15,25,200}:Color{8,12,20,200};
        DrawRectangle(0,(int)ry,SCREEN_W-220,34,rowbg);
        DrawRectangle(8,(int)(ry+8),20,20,{td.r,td.g,td.b,255});
        DrawText(td.name,34,(int)(ry+10),13,C_PARCHMENT);
        DrawText(TextFormat("%d soldiers",cnt),280,(int)(ry+10),12,C_SECONDARY);
        // Checkbox
        bool sel=g_preBattle.include[i];
        Rectangle cb={(float)(SCREEN_W-210),(float)(ry+6),22,22};
        bool chv=ptInRect(mouse,cb);
        DrawRectangleRec(cb,sel?Color{30,80,30,255}:chv?Color{22,48,22,255}:Color{14,28,14,220});
        DrawRectangleLinesEx(cb,1,sel?C_GOLD:Color{60,90,40,255});
        if(sel) DrawText("✓",(int)(cb.x+5),(int)(cb.y+3),14,C_GOLD);
        if(chv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
            if(!sel&&deployCount<12) g_preBattle.include[i]=true;
            else if(sel) g_preBattle.include[i]=false;
        }
        // Stats brief
        DrawText(TextFormat("Atk:%d Def:%d HP:%.0f",td.meleeAttack,td.meleeDefense,
                 td.hpPerSoldier*cnt),(int)(SCREEN_W-190),(int)(ry+10),11,C_SECONDARY);
    }

    // Bottom half: enemy info
    DrawRectangle(0,(int)(50+halfH),SCREEN_W,(int)halfH,{20,8,8,200});
    DrawText("ENEMY FORCES:",14,(int)(58+halfH),14,C_ENEMY_COL);
    if(g_preBattle.fogOfWar){
        DrawText(TextFormat("Estimated strength: ~%d soldiers",g_preBattle.estimatedEnemyStrength),
                 14,(int)(84+halfH),14,C_SECONDARY);
        DrawText("(Scout reports unreliable — deploy explorers for better intel)",
                 14,(int)(106+halfH),12,{100,80,60,255});
    } else {
        // Show actual army
        int pi=g_preBattle.provinceIdx;
        if(pi>=0&&pi<(int)g_campaign.provinces.size()){
            for(int i=0;i<(int)g_campaign.provinces[pi].army.size();i++){
                auto [ti,cnt]=g_campaign.provinces[pi].army[i];
                if(ti>=unitTypeCount()) continue;
                const UnitTypeDef& td=g_unitTypes[ti];
                float ry=84.f+halfH+i*28.f;
                DrawRectangle(8,(int)(ry+4),16,16,{td.r,td.g,td.b,100});
                DrawText(td.name,30,(int)(ry+6),13,C_PARCHMENT);
                DrawText(TextFormat("x%d",cnt),280,(int)(ry+6),12,C_ENEMY_COL);
            }
        }
    }

    // Bottom buttons
    int bY=SCREEN_H-60;
    DrawRectangle(0,bY,SCREEN_W,60,{0,0,0,200});
    DrawText(TextFormat("Deploying: %d / 12 groups",deployCount),14,(int)(bY+22),13,C_SECONDARY);

    bool canStart=(deployCount>0);
    Color cn=canStart?Color{30,65,30,255}:Color{30,30,30,255};
    Color ch=canStart?Color{55,120,50,255}:Color{30,30,30,255};
    if(drawButton({(float)(SCREEN_W-450),(float)(bY+6),240,46},"START BATTLE",mouse,cn,ch)&&canStart){
        // Build player groups from selection
        std::vector<std::pair<int,int>> playerGroups, enemyGroups;
        for(int i=0;i<(int)g_campaign.readyUnits.size();i++){
            if(i<(int)g_preBattle.include.size()&&g_preBattle.include[i])
                playerGroups.push_back(g_campaign.readyUnits[i]);
        }
        int pi=g_preBattle.provinceIdx;
        if(pi>=0&&pi<(int)g_campaign.provinces.size())
            enemyGroups=g_campaign.provinces[pi].army;
        char sname[64];
        snprintf(sname,63,"Battle of %s",
                 (pi>=0&&pi<(int)g_campaign.provinces.size())?g_campaign.provinces[pi].name:"Unknown");
        TerrainType ter=(pi>=0&&pi<(int)g_campaign.provinces.size())?
                         g_campaign.provinces[pi].terrain:TERRAIN_PLAIN;
        initBattle(playerGroups,enemyGroups,ter,pi,g_preBattle.isDefense,sname);
        return STATE_BATTLE;
    }
    if(drawButton({(float)(SCREEN_W-200),(float)(bY+6),186,46},"RETREAT",mouse,{55,20,20,255},{90,35,35,255})){
        if(g_preBattle.isDefense&&g_preBattle.provinceIdx>=0&&
           g_preBattle.provinceIdx<(int)g_campaign.provinces.size()){
            // Lose province
            g_campaign.provinces[g_preBattle.provinceIdx].owner=FACTION_NEUTRAL;
        }
        return STATE_CAMPAIGN_MAP;
    }
    return STATE_PRE_BATTLE;
}

// ═══════════════════════════════════════════════════════════════════════════
//  STATE: CAMPAIGN MAP
// ═══════════════════════════════════════════════════════════════════════════
static GameState updateDrawCampaignMap(Vector2 mouse,float dt){
    (void)dt;
    updateProvinceCenters();
    ClearBackground({10,12,8,255});
    // Fog / atmosphere rectangles (animated)
    static float fogX=0;
    fogX+=10.f*dt;
    for(int i=0;i<8;i++){
        float fx=fmodf(fogX+i*200.f,(float)SCREEN_W+400)-200;
        DrawRectangle((int)fx,0,180,SCREEN_H,{15,20,12,(unsigned char)(8+i*3)});
    }

    // Draw adjacencies
    for(int i=0;i<(int)g_campaign.provinces.size();i++){
        auto& p=g_campaign.provinces[i];
        for(int adj:p.adjacent){
            if(adj>i)
                DrawLineEx(p.center,g_campaign.provinces[adj].center,2.f,{40,55,35,100});
        }
    }

    // Update explored provinces (6.3)
    for(int i=0;i<(int)g_campaign.provinces.size()&&i<32;i++){
        if(g_campaign.provinces[i].owner==FACTION_PLAYER) g_campaign.explored[i]=true;
        // Adjacent to player province are also explored
        for(int adj:g_campaign.provinces[g_campaign.playerProvince].adjacent){
            if(adj<32) g_campaign.explored[adj]=true;
        }
        if((int)g_campaign.playerProvince<32) g_campaign.explored[g_campaign.playerProvince]=true;
    }

    // Draw provinces
    for(int i=0;i<(int)g_campaign.provinces.size();i++){
        auto& p=g_campaign.provinces[i];
        bool explored=(i<32&&g_campaign.explored[i]);
        Color oc=factionColors[(int)p.owner];
        // 6.3: Unexplored provinces rendered as dark silhouettes
        if(!explored) oc={40,40,40,255};
        float rad=30.f;
        DrawCircleV(p.center,(int)rad,{oc.r,oc.g,oc.b,80});
        DrawCircleLines((int)p.center.x,(int)p.center.y,(int)rad,oc);
        // 4.5: Selected province pulse
        if(g_selectedProvince==i){
            float pulse=rad+4.f+sinf(g_menuTime*4.f)*4.f;
            DrawCircleLines((int)p.center.x,(int)p.center.y,(int)pulse,C_GOLD);
        }
        // Terrain indicator
        static const char* terrIcons[4]={"~","T","^","W"};
        DrawText(terrIcons[(int)p.terrain],(int)(p.center.x-5),(int)(p.center.y-8),18,{200,200,160,200});
        // Name
        int tw=MeasureText(p.name,11);
        DrawText(p.name,(int)(p.center.x-tw/2),(int)(p.center.y+rad+4),11,explored?C_PARCHMENT:Color{100,100,100,255});
        // Player marker
        if(i==g_campaign.playerProvince){
            DrawCircleLines((int)p.center.x,(int)p.center.y,(int)(rad+6),C_ALLY);
            DrawText("YOU",(int)(p.center.x-12),(int)(p.center.y+rad+18),11,C_ALLY);
        }
        // Army indicator — only if explored (6.3)
        if(!p.army.empty()&&explored){
            int total=0; for(auto [ti,c]:p.army) total+=c;
            DrawText(TextFormat("⚔%d",total),(int)(p.center.x-12),(int)(p.center.y-rad-16),11,
                     p.owner==FACTION_PLAYER?C_ALLY:C_ENEMY_COL);
        } else if(!p.army.empty()&&!explored){
            DrawText("?",(int)(p.center.x-4),(int)(p.center.y-rad-16),11,{80,80,80,255});
        }
        // Hover tooltip
        if(vdist(mouse,p.center)<rad+6){
            // Highlight adjacent
            for(int adj:p.adjacent){
                DrawLineEx(p.center,g_campaign.provinces[adj].center,2.f,C_GOLD);
                DrawCircleLines((int)g_campaign.provinces[adj].center.x,
                                (int)g_campaign.provinces[adj].center.y,
                                (int)rad+4,{C_GOLD.r,C_GOLD.g,C_GOLD.b,80});
            }
            // Tooltip
            float tx=mouse.x+10, ty=mouse.y-40;
            DrawRectangle((int)tx-4,(int)ty-4,220,90,{0,0,0,200});
            DrawRectangleLinesEx({tx-4,ty-4,220,90},1,C_GOLD);
            DrawText(p.name,(int)tx,(int)ty,14,C_GOLD); ty+=18;
            DrawText(TextFormat("Terrain: %s",terrainNames[(int)p.terrain]),(int)tx,(int)ty,12,C_SECONDARY); ty+=15;
            DrawText(TextFormat("Owner: %s",factionNames[(int)p.owner]),(int)tx,(int)ty,12,oc); ty+=15;
            if(p.hasCity) DrawText(TextFormat("City: %s",p.city.name),(int)tx,(int)ty,12,C_PARCHMENT); ty+=15;
            int tot=0; for(auto [ti,c]:p.army) tot+=c;
            if(tot>0) DrawText(TextFormat("Army: ~%d soldiers",tot),(int)tx,(int)ty,12,C_ENEMY_COL);

            // Click to interact
            if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                g_selectedProvince=i; // 4.5: track selected province
                // Check if adjacent to player
                bool isAdj=false;
                for(int adj:g_campaign.provinces[g_campaign.playerProvince].adjacent)
                    if(adj==i) isAdj=true;
                bool isPlayer=(i==g_campaign.playerProvince);

                if(isPlayer&&p.hasCity){
                    // View city
                    g_campaign.viewedCity=i;
                    return STATE_CITY_MANAGEMENT;
                } else if(isAdj){
                    if(p.owner==FACTION_PLAYER){
                        // Move to this province and process turn (movement costs a turn)
                        g_campaign.playerProvince=i;
                        processTurn();
                        // Check if we triggered a defense battle
                        if(!g_campaign.provinces[i].army.empty()){
                            g_preBattle.provinceIdx=i;
                            g_preBattle.isDefense=true;
                            g_preBattle.fogOfWar=false;
                            g_preBattle.estimatedEnemyStrength=0;
                            for(auto [ti,c]:g_campaign.provinces[i].army) g_preBattle.estimatedEnemyStrength+=c;
                            g_preBattle.include.clear();
                            g_preBattle.include.resize(g_campaign.readyUnits.size(),true);
                            for(int j=12;j<(int)g_preBattle.include.size();j++)
                                g_preBattle.include[j]=false;
                            return STATE_PRE_BATTLE;
                        }
                    } else {
                        // Initiate battle with enemy province
                        g_preBattle.provinceIdx=i;
                        g_preBattle.isDefense=false;
                        g_preBattle.fogOfWar=(p.owner!=FACTION_NEUTRAL);
                        g_preBattle.estimatedEnemyStrength=0;
                        for(auto [ti,c]:p.army) g_preBattle.estimatedEnemyStrength+=c;
                        g_preBattle.estimatedEnemyStrength+=(int)((frandMT()-0.5f)*20);
                        g_preBattle.include.clear();
                        g_preBattle.include.resize(g_campaign.readyUnits.size(),false);
                        // Auto-select all by default
                        for(int j=0;j<std::min((int)g_preBattle.include.size(),12);j++)
                            g_preBattle.include[j]=true;
                        return STATE_PRE_BATTLE;
                    }
                }
            }
        }
    }

    // Top bar: resources
    DrawRectangle(0,0,SCREEN_W,36,{0,0,0,210});
    DrawText(TextFormat("Turn: %d",g_campaign.turn),10,10,14,C_GOLD);
    Resources& r=g_campaign.res;
    DrawText(TextFormat("Gold: %.0f  Food: %.0f  Wood: %.0f  Stone: %.0f  Iron: %.0f",
             r.gold,r.food,r.wood,r.stone,r.iron),100,10,13,C_PARCHMENT);

    // Bottom buttons
    DrawRectangle(0,SCREEN_H-50,SCREEN_W,50,{0,0,0,210});
    DrawLine(0,SCREEN_H-50,SCREEN_W,SCREEN_H-50,{80,65,30,160});

    if(drawSmBtn({10,(float)(SCREEN_H-42),140,34},"END TURN",mouse,{20,50,20,255},{40,90,38,255})){
        // Process turn (in case player wants to skip without moving)
        processTurn();
        // Check victory/defeat after processing
        int totalProv=(int)g_campaign.provinces.size();
        int playerProv=0;
        for(auto& p:g_campaign.provinces) if(p.owner==FACTION_PLAYER) playerProv++;
        if(playerProv>=(int)(totalProv*0.8f)){
            return STATE_VICTORY;
        }
        if(playerProv==0){
            return STATE_DEFEAT;
        }
    }
    if(drawSmBtn({160,(float)(SCREEN_H-42),120,34},"CODEX",mouse)) return STATE_UNIT_CODEX;
    if(drawSmBtn({290,(float)(SCREEN_H-42),120,34},"RECRUIT",mouse)){
        g_campaign.viewedCity=g_campaign.playerProvince;
        return STATE_RECRUITMENT;
    }
    if(drawSmBtn({420,(float)(SCREEN_H-42),140,34},"MARKETPLACE",mouse)) return STATE_MARKETPLACE;
    if(drawSmBtn({(float)(SCREEN_W-130),(float)(SCREEN_H-42),120,34},"MENU",mouse,
                  {50,20,20,255},{90,38,38,255})) return STATE_MAIN_MENU;

    return STATE_CAMPAIGN_MAP;
}

// ═══════════════════════════════════════════════════════════════════════════
//  STATE: CITY MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════
static GameState updateDrawCityManagement(Vector2 mouse){
    if(g_campaign.viewedCity<0||g_campaign.viewedCity>=(int)g_campaign.provinces.size())
        return STATE_CAMPAIGN_MAP;

    Province& prov=g_campaign.provinces[g_campaign.viewedCity];
    if(!prov.hasCity) return STATE_CAMPAIGN_MAP;
    City& city=prov.city;
    Resources& res=g_campaign.res;

    ClearBackground({12,10,8,255});
    // City illustration (procedural schematic)
    // Draw silhouette shapes
    DrawRectangle(0,0,SCREEN_W,120,{20,16,10,255});
    // Towers
    for(int t=0;t<5;t++){
        int tx=100+t*220;
        DrawRectangle(tx,20,40,100,{55,50,42,255});
        // Battlements
        for(int b=0;b<3;b++) DrawRectangle(tx+b*14,12,10,10,{55,50,42,255});
        // Window
        DrawRectangle(tx+14,50,12,18,{120,100,60,60});
    }
    // Walls connecting
    DrawRectangle(0,95,SCREEN_W,25,{45,40,32,255});
    DrawLine(0,96,SCREEN_W,96,{70,60,40,200});

    DrawText(TextFormat("City of %s",city.name),14,10,24,C_GOLD);
    DrawLine(0,124,SCREEN_W,124,{80,65,30,150});

    // Building grid (5x3 = 15 slots, but BLD_COUNT=14)
    float gridX=14, gridY=132;
    float cellW=(SCREEN_W-28)/5.f, cellH=110.f;

    for(int b=0;b<BLD_COUNT;b++){
        int col=b%5, row=b/5;
        float bx=gridX+col*cellW;
        float by=gridY+row*cellH;
        float bw=cellW-6, bh=cellH-6;

        bool built=city.built[b];
        bool constructing=(city.constructing==b);
        bool canBuild=!built&&!constructing;

        // Check prereq
        // Fix: use the fixed prereq table below
        static const int prereqs[BLD_COUNT]={-1,-1,1,-1,2,-1,5,5,4,-1,9,-1,-1,-1};
        bool prereqOk=(prereqs[b]<0||city.built[prereqs[b]]);
        // Check coast requirement for port
        bool terrainOk=(b==BLD_PORT)?(prov.terrain==TERRAIN_COAST):true;
        canBuild=canBuild&&prereqOk&&terrainOk;

        // Cost check
        bool canAfford=(res.gold>=bldGoldCost[b]&&res.wood>=bldWoodCost[b]
                        &&res.stone>=bldStoneCost[b]&&res.iron>=bldIronCost[b]);
        bool noOtherConstruction=(city.constructing<0);

        Color bgCol=built?Color{20,40,20,220}:constructing?Color{30,30,50,220}:Color{20,16,12,220};
        bool hover=ptInRect(mouse,{bx,by,bw,bh});
        if(hover&&!built) bgCol.r=clampU8(bgCol.r+15);
        DrawRectangleRec({bx,by,bw,bh},bgCol);
        DrawRectangleLinesEx({bx,by,bw,bh},1,built?C_GOLD:constructing?C_ALLY:Color{50,45,35,255});

        // Icon color patch
        Color ic=built?C_GOLD:constructing?C_ALLY:Color{80,70,50,255};
        DrawRectangle((int)(bx+4),(int)(by+4),18,18,ic);
        DrawText(bldNames[b],(int)(bx+26),(int)(by+6),11,built?C_GOLD:C_SECONDARY);

        if(built){
            // Show yield
            char yield[64]="";
            if(bldGoldYield[b]>0) snprintf(yield,63,"+%.0fg",bldGoldYield[b]);
            else if(bldFoodYield[b]>0) snprintf(yield,63,"+%.0ff",bldFoodYield[b]);
            else if(bldWoodYield[b]>0) snprintf(yield,63,"+%.0fw",bldWoodYield[b]);
            else if(bldStoneYield[b]>0) snprintf(yield,63,"+%.0fs",bldStoneYield[b]);
            else if(bldIronYield[b]>0) snprintf(yield,63,"+%.0fi",bldIronYield[b]);
            else strcpy(yield,"active");
            DrawText(yield,(int)(bx+6),(int)(by+28),12,{100,200,100,255});
        } else if(constructing){
            DrawText(TextFormat("%d turns",city.constructTurns),(int)(bx+6),(int)(by+28),12,{100,140,220,255});
        } else {
            // Cost
            char costStr[80]="";
            if(bldGoldCost[b]) snprintf(costStr+strlen(costStr),79-strlen(costStr)," %dG",bldGoldCost[b]);
            if(bldWoodCost[b]) snprintf(costStr+strlen(costStr),79-strlen(costStr)," %dW",bldWoodCost[b]);
            if(bldStoneCost[b]) snprintf(costStr+strlen(costStr),79-strlen(costStr)," %dS",bldStoneCost[b]);
            if(bldIronCost[b]) snprintf(costStr+strlen(costStr),79-strlen(costStr)," %dI",bldIronCost[b]);
            DrawText(costStr,(int)(bx+4),(int)(by+28),10,canAfford?Color{180,200,100,255}:Color{200,100,80,255});
            DrawText(TextFormat("%dt",bldTurns[b]),(int)(bx+4),(int)(by+42),11,C_SECONDARY);
        }

        // Build button
        if(canBuild&&noOtherConstruction){
            Rectangle buildBtn={bx+4,by+bh-26,bw-8,22};
            if(drawSmBtn(buildBtn,"BUILD",mouse,
                         canAfford?Color{20,50,20,255}:Color{30,20,20,255},
                         canAfford?Color{40,90,38,255}:Color{30,20,20,255})&&canAfford){
                res.gold-=bldGoldCost[b];
                res.wood-=bldWoodCost[b];
                res.stone-=bldStoneCost[b];
                res.iron-=bldIronCost[b];
                city.constructing=b;
                city.constructTurns=bldTurns[b];
            }
        }
        if(!prereqOk&&!built&&hover){
            // Prereq tooltip
            int pi=prereqs[b];
            if(pi>=0&&pi<BLD_COUNT)
                DrawText(TextFormat("Requires: %s",bldNames[pi]),
                         (int)(bx+4),(int)(by+bh-26),10,{200,120,60,255});
        }
        if(!terrainOk&&!built){
            DrawText("Coastal only",(int)(bx+4),(int)(by+bh-26),10,{200,120,60,255});
        }
    }

    // Bottom bar
    DrawRectangle(0,SCREEN_H-44,SCREEN_W,44,{0,0,0,210});
    DrawLine(0,SCREEN_H-44,SCREEN_W,SCREEN_H-44,{80,65,30,160});
    DrawText(TextFormat("Gold: %.0f  Food: %.0f  Wood: %.0f  Stone: %.0f  Iron: %.0f",
             res.gold,res.food,res.wood,res.stone,res.iron),14,SCREEN_H-34,13,C_PARCHMENT);

    if(drawSmBtn({(float)(SCREEN_W-260),(float)(SCREEN_H-38),120,30},"RECRUIT",mouse))
        return STATE_RECRUITMENT;
    if(drawSmBtn({(float)(SCREEN_W-130),(float)(SCREEN_H-38),116,30},"BACK",mouse,
                  {50,25,25,255},{80,40,40,255}))
        return STATE_CAMPAIGN_MAP;

    return STATE_CITY_MANAGEMENT;
}

// ═══════════════════════════════════════════════════════════════════════════
//  STATE: RECRUITMENT
// ═══════════════════════════════════════════════════════════════════════════
static GameState updateDrawRecruitment(Vector2 mouse){
    if(g_campaign.viewedCity<0||g_campaign.viewedCity>=(int)g_campaign.provinces.size())
        return STATE_CAMPAIGN_MAP;
    Province& prov=g_campaign.provinces[g_campaign.viewedCity];
    City& city=prov.city;
    Resources& res=g_campaign.res;

    ClearBackground(C_BG);
    for(int gx=0;gx<SCREEN_W;gx+=40) DrawLine(gx,0,gx,SCREEN_H,{18,14,10,50});
    for(int gy=0;gy<SCREEN_H;gy+=40) DrawLine(0,gy,SCREEN_W,gy,{18,14,10,50});

    DrawRectangle(0,0,SCREEN_W,50,{0,0,0,220});
    DrawText(TextFormat("RECRUITMENT — %s",city.name),14,10,24,C_GOLD);
    DrawText(TextFormat("Turn: %d  |  Gold: %.0f  Food: %.0f  Iron: %.0f",
             g_campaign.turn,res.gold,res.food,res.iron),14,36,12,C_SECONDARY);

    // Bit requirements mapping
    // buildingReqs bits: 0=Barracks,1=Stable,2=Range(idx=7),3=Smithy(idx=4),4=Workshop(idx=8)
    auto hasReq=[&](int reqs)->bool{
        if(reqs&1) if(!city.built[BLD_BARRACKS]) return false;
        if(reqs&2) if(!city.built[BLD_STABLE]) return false;
        if(reqs&4) if(!city.built[BLD_RANGE]) return false;
        if(reqs&8) if(!city.built[BLD_SMITHY]) return false;
        if(reqs&16) if(!city.built[BLD_WORKSHOP]) return false;
        return true;
    };

    // Left: available units
    float leftW=SCREEN_W*0.55f;
    DrawText("Available for recruitment:",(int)14,(int)60,13,C_PARCHMENT);

    for(int t=0;t<unitTypeCount();t++){
        const UnitTypeDef& td=g_unitTypes[t];
        float ry=80.f+t*38.f;
        if(ry+38>SCREEN_H-60) break;
        bool unlocked=hasReq(td.buildingReqs);

        Color rowbg=(t%2==0)?Color{18,14,10,200}:Color{14,10,8,200};
        DrawRectangle(0,(int)ry,(int)leftW,36,rowbg);
        Color col2={td.r,td.g,td.b,(unsigned char)(unlocked?220:80)};
        DrawRectangle(6,(int)(ry+8),18,18,col2);
        DrawText(td.name,(int)28,(int)(ry+10),13,unlocked?C_PARCHMENT:C_SECONDARY);

        if(unlocked){
            DrawText(TextFormat("G:%d F:%d I:%d",td.recruitGold,td.recruitFood,td.recruitIron),
                     (int)(leftW-280),(int)(ry+8),11,C_SECONDARY);
            DrawText(TextFormat("%d turns",td.recruitTurns),(int)(leftW-150),(int)(ry+8),11,C_SECONDARY);
            DrawText(TextFormat("%d soldiers",td.soldierCount),(int)(leftW-150),(int)(ry+22),11,C_SECONDARY);
            bool canAfford=(res.gold>=td.recruitGold&&res.food>=td.recruitFood&&res.iron>=td.recruitIron);
            Rectangle addBtn={leftW-80,ry+6,72,26};
            if(drawSmBtn(addBtn,"RECRUIT",mouse,
                         canAfford?Color{20,50,20,255}:Color{30,20,20,255},
                         canAfford?Color{40,90,38,255}:Color{30,20,20,255})&&canAfford){
                res.gold-=td.recruitGold; res.food-=td.recruitFood; res.iron-=td.recruitIron;
                g_campaign.recruitQueue.push_back({t,td.recruitTurns,td.recruitTurns});
            }
        } else {
            // Show requirements
            std::string req="Requires: ";
            if(td.buildingReqs&1) req+=std::string(bldNames[BLD_BARRACKS])+", ";
            if(td.buildingReqs&2) req+=std::string(bldNames[BLD_STABLE])+", ";
            if(td.buildingReqs&4) req+=std::string(bldNames[BLD_RANGE])+", ";
            if(td.buildingReqs&8) req+=std::string(bldNames[BLD_SMITHY])+", ";
            if(td.buildingReqs&16) req+=std::string(bldNames[BLD_WORKSHOP])+", ";
            if(req.size()>2&&req.back()==' ') req=req.substr(0,req.size()-2);
            DrawText(req.c_str(),(int)(leftW-350),(int)(ry+10),10,{140,100,60,255});
        }
    }

    // Right: queue + ready
    float rx=leftW+10;
    DrawLine((int)rx,50,(int)rx,SCREEN_H-50,{60,50,35,120});
    float rightW=SCREEN_W-rx-10;
    DrawText("Recruitment queue:",(int)(rx+10),(int)60,13,C_PARCHMENT);
    for(int i=0;i<(int)g_campaign.recruitQueue.size();i++){
        auto& e=g_campaign.recruitQueue[i];
        if(e.typeIdx>=unitTypeCount()) continue;
        float ry=80.f+i*30.f;
        if(ry+30>SCREEN_H/2) break;
        DrawText(g_unitTypes[e.typeIdx].name,(int)(rx+10),(int)(ry+6),13,C_ALLY);
        DrawText(TextFormat("%d turn(s)",e.turnsLeft),(int)(rx+rightW-80),(int)(ry+6),12,C_SECONDARY);
        // 4.3: Progress bar
        int orig=e.originalTurns>0?e.originalTurns:1;
        float progress=1.f-(float)e.turnsLeft/(float)orig;
        float pbW=rightW-100.f;
        DrawRectangle((int)(rx+10),(int)(ry+20),(int)pbW,5,DARKGRAY);
        DrawRectangle((int)(rx+10),(int)(ry+20),(int)(pbW*progress),5,{80,160,80,255});
    }
    if(g_campaign.recruitQueue.empty())
        DrawText("(empty)",(int)(rx+10),(int)100,12,C_SECONDARY);

    DrawLine((int)rx,SCREEN_H/2,(int)SCREEN_W,SCREEN_H/2,{60,50,35,80});
    DrawText("Ready to deploy:",(int)(rx+10),(int)(SCREEN_H/2+8),13,C_PARCHMENT);
    for(int i=0;i<(int)g_campaign.readyUnits.size();i++){
        auto [ti,cnt]=g_campaign.readyUnits[i];
        if(ti>=unitTypeCount()) continue;
        const UnitTypeDef& td2=g_unitTypes[ti];
        float ry=SCREEN_H/2+30.f+i*28.f;
        if(ry+28>SCREEN_H-50) break;
        DrawRectangle(6+(int)rx,(int)(ry+4),14,14,{td2.r,td2.g,td2.b,255});
        DrawText(td2.name,(int)(rx+26),(int)(ry+6),13,C_PARCHMENT);
        DrawText(TextFormat("x%d",cnt),(int)(rx+rightW-50),(int)(ry+6),12,C_SECONDARY);
    }

    // Bottom
    DrawRectangle(0,SCREEN_H-50,SCREEN_W,50,{0,0,0,210});
    if(drawSmBtn({(float)(SCREEN_W-130),(float)(SCREEN_H-42),116,32},"BACK",mouse,
                  {50,25,25,255},{80,40,40,255}))
        return STATE_CITY_MANAGEMENT;
    return STATE_RECRUITMENT;
}

// ═══════════════════════════════════════════════════════════════════════════
//  STATE: UNIT CODEX
// ═══════════════════════════════════════════════════════════════════════════
static int g_codexIdx=0;
static float g_codexAngle=0;
static GameState updateDrawUnitCodex(Vector2 mouse,float dt){
    g_codexAngle+=45.f*dt;

    ClearBackground({8,8,16,255});
    for(int gx=0;gx<SCREEN_W;gx+=40) DrawLine(gx,0,gx,SCREEN_H,{16,16,32,60});
    for(int gy=0;gy<SCREEN_H;gy+=40) DrawLine(0,gy,SCREEN_W,gy,{16,16,32,60});

    DrawRectangle(0,0,SCREEN_W,50,{0,0,0,220});
    DrawText("UNIT CODEX",14,10,28,C_GOLD);
    DrawText("Encyclopedia of all military units",14,38,12,C_SECONDARY);

    float listW=260, listX=10, contentX=listW+20;
    int nc=unitTypeCount();

    // Left list
    DrawRectangle((int)listX,52,(int)listW,SCREEN_H-52-44,{12,10,8,210});
    DrawRectangleLinesEx({listX,52,listW,(float)(SCREEN_H-52-44)},1,{60,50,35,200});

    for(int i=0;i<nc;i++){
        const UnitTypeDef& td=g_unitTypes[i];
        bool sel=(i==g_codexIdx);
        float ry=60.f+i*38.f;
        if(ry+38>SCREEN_H-44) break;
        Color rowbg=sel?Color{25,50,70,220}:(i%2==0)?Color{16,13,10,200}:Color{12,10,8,200};
        bool hv=ptInRect(mouse,{listX,ry,listW,36});
        if(hv&&!sel) rowbg={22,20,16,200};
        DrawRectangleRec({listX,ry,listW,36},rowbg);
        if(sel) DrawRectangleLinesEx({listX,ry,listW,36},1,C_GOLD);
        DrawRectangle((int)(listX+4),(int)(ry+8),16,16,{td.r,td.g,td.b,255});
        DrawText(td.name,(int)(listX+26),(int)(ry+10),13,sel?C_GOLD:C_PARCHMENT);
        if(hv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) g_codexIdx=i;
    }

    // Right content
    if(g_codexIdx>=nc) return STATE_UNIT_CODEX; // 1.6: bounds check
    if(g_codexIdx<nc){
        const UnitTypeDef& td=g_unitTypes[g_codexIdx];
        float cx=contentX, cy=58.f;

        DrawText(td.name,(int)cx,(int)cy,22,C_GOLD); cy+=30;
        DrawLine((int)cx,(int)cy,(int)(SCREEN_W-10),(int)cy,{80,65,30,120}); cy+=10;

        // Sprite
        Vector2 sprCenter={(float)(SCREEN_W-100),(float)140};
        drawSoldierSprite(g_playerTextures[g_codexIdx],sprCenter,g_codexAngle,2.5f,
                          {td.r,td.g,td.b,255},false);
        DrawCircleLines((int)sprCenter.x,(int)sprCenter.y,55,{80,70,50,80});

        // Lore — word wrapped (3.8)
        drawWrappedText(td.lore,(int)cx,(int)cy,(int)(SCREEN_W-contentX-120),12,C_SECONDARY);
        cy+=30;

        // Stats table
        DrawText(TextFormat("Soldiers: %d",td.soldierCount),(int)cx,(int)cy,13,C_PARCHMENT); cy+=18;
        DrawText(TextFormat("HP per soldier: %.0f",td.hpPerSoldier),(int)cx,(int)cy,13,C_PARCHMENT); cy+=18;
        DrawText(TextFormat("Armor: %d / Speed: %d",td.armor,td.speed),(int)cx,(int)cy,13,C_PARCHMENT); cy+=18;
        DrawText(TextFormat("Melee: Atk%d Def%d Dmg%.0f+%.0f (%.1fs)",
                 td.meleeAttack,td.meleeDefense,td.meleeBaseDmg,td.meleeAPDmg,td.meleeInterval),
                 (int)cx,(int)cy,13,{220,180,60,255}); cy+=18;
        if(td.range>0){
            DrawText(TextFormat("Range: %dpx  Dmg%.0f+%.0f (%.1fs reload)",
                     td.range,td.missileBaseDmg,td.missileAPDmg,td.missileReload),
                     (int)cx,(int)cy,13,{180,100,200,255}); cy+=18;
        }
        DrawText(TextFormat("Morale: %.0f",td.morale),(int)cx,(int)cy,13,{200,200,80,255}); cy+=18;
        cy+=8;

        // Recruitment cost
        DrawText("Recruitment:",(int)cx,(int)cy,13,C_GOLD); cy+=16;
        DrawText(TextFormat("Gold: %d  Food: %d  Iron: %d  Turns: %d",
                 td.recruitGold,td.recruitFood,td.recruitIron,td.recruitTurns),
                 (int)cx,(int)cy,12,C_SECONDARY); cy+=16;
        DrawText(TextFormat("Maintenance: %d gold/turn, %d food/turn",td.maintGold,td.maintFood),
                 (int)cx,(int)cy,12,C_SECONDARY); cy+=20;

        // Stat bars
        DrawText("Stats (normalized):",(int)cx,(int)cy,12,{100,140,200,255}); cy+=14;
        drawStatBars(cx,cy,(float)(SCREEN_W-contentX-20),140.f,td);
    }

    // Bottom
    DrawRectangle(0,SCREEN_H-44,SCREEN_W,44,{0,0,0,210});
    if(drawSmBtn({(float)(SCREEN_W-130),(float)(SCREEN_H-38),116,30},"BACK",mouse,
                  {50,25,25,255},{80,40,40,255})) return STATE_CAMPAIGN_MAP;

    return STATE_UNIT_CODEX;
}

// ═══════════════════════════════════════════════════════════════════════════
//  STATE: SETTINGS
// ═══════════════════════════════════════════════════════════════════════════
static GameState updateDrawSettings(Vector2 mouse){
    ClearBackground(C_BG);
    DrawRectangleGradientV(0,0,SCREEN_W,SCREEN_H,{10,8,6,255},C_BG);
    int tw2=MeasureText("SETTINGS",32);
    DrawText("SETTINGS",SCREEN_W/2-tw2/2,30,32,C_GOLD);
    DrawLine(SCREEN_W/2-200,72,SCREEN_W/2+200,72,C_GOLD);

    float cx=(float)(SCREEN_W/2-250), cw=500.f, cy=90.f;

    // Master Volume
    DrawText("Master Volume",(int)cx,(int)(cy-14),12,C_SECONDARY);
    g_settings.masterVolume=drawFloatSlider({cx,cy,cw,14},g_settings.masterVolume,0.f,1.f,"","%.2f",mouse,{80,160,80,200});
    cy+=36;

    // Music Volume
    DrawText("Music Volume",(int)cx,(int)(cy-14),12,C_SECONDARY);
    g_settings.musicVolume=drawFloatSlider({cx,cy,cw,14},g_settings.musicVolume,0.f,1.f,"","%.2f",mouse,{80,120,180,200});
    cy+=36;

    // Difficulty
    DrawText("Difficulty:",(int)cx,(int)cy,14,C_SECONDARY); cy+=20;
    static const char* diffNames[]={"EASY","NORMAL","HARD"};
    for(int d=0;d<3;d++){
        float bx=cx+d*120.f;
        bool sel=(g_settings.difficulty==d);
        Color bc=sel?Color{40,80,40,255}:Color{20,30,20,255};
        Color bh=sel?Color{60,120,60,255}:Color{35,55,35,255};
        if(drawSmBtn({bx,cy,110,30},diffNames[d],mouse,bc,bh)) g_settings.difficulty=d;
        if(sel) DrawRectangleLinesEx({bx,cy,110,30},2,C_GOLD);
    }
    cy+=46;

    // Show FPS checkbox
    Rectangle cbr={(float)cx,cy,20,20};
    bool cbhv=ptInRect(mouse,cbr);
    DrawRectangleRec(cbr,g_settings.showFPS?Color{40,80,40,255}:cbhv?Color{25,45,25,255}:Color{15,25,15,220});
    DrawRectangleLinesEx(cbr,1,g_settings.showFPS?C_GOLD:Color{60,90,40,255});
    if(g_settings.showFPS) DrawText("✓",(int)(cx+4),(int)(cy+2),14,C_GOLD);
    DrawText("Show FPS Counter",(int)(cx+28),(int)(cy+2),14,C_SECONDARY);
    if(cbhv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) g_settings.showFPS=!g_settings.showFPS;
    cy+=36;

    // F11 hint
    DrawText("F11 / Alt+Enter: Toggle Fullscreen",(int)cx,(int)cy,13,C_PARCHMENT);
    cy+=28;

    // Save button
    if(drawButton({(float)(SCREEN_W/2-160),(float)(cy+10),160,44},"SAVE",mouse,{30,60,30,255},{50,100,50,255})){
        saveSettings();
    }
    if(drawButton({(float)(SCREEN_W/2+8),(float)(cy+10),160,44},"BACK",mouse,{50,25,25,255},{90,40,40,255})){
        saveSettings();
        return STATE_MAIN_MENU;
    }
    return STATE_SETTINGS;
}

// ═══════════════════════════════════════════════════════════════════════════
//  STATE: UNIT EDITOR (sandbox, from main menu)
// ═══════════════════════════════════════════════════════════════════════════
static Texture2D g_editorPrevTex={0};
static void rebuildEditorPreview(){
    if(g_editorPrevTex.id>0) UnloadTexture(g_editorPrevTex);
    g_editorPrevTex={0};
    if(g_editTypeIdx>=0&&g_editTypeIdx<unitTypeCount()){
        const UnitTypeDef& td=g_unitTypes[g_editTypeIdx];
        Image img;
        switch(td.spriteBase){
            case SPR_CAVALRY: img=makeCavalrySprite(td.r,td.g,td.b); break;
            case SPR_RANGED:  img=makeRangedSprite(td.r,td.g,td.b,td.missileReload>2.5f); break;
            default:          img=makeInfantrySprite(td.r,td.g,td.b,td.weaponHint); break;
        }
        g_editorPrevTex=imageToTex(img);
    }
}

static GameState updateDrawUnitEditor(Vector2 mouse,float dt){
    if(g_editTypeIdx>=unitTypeCount()) g_editTypeIdx=0;
    g_editorPreviewAngle+=40.f*dt;

    ClearBackground({8,10,18,255});
    for(int gx=0;gx<SCREEN_W;gx+=40) DrawLine(gx,0,gx,SCREEN_H,{18,20,40,70});
    for(int gy=0;gy<SCREEN_H;gy+=40) DrawLine(0,gy,SCREEN_W,gy,{18,20,40,70});

    DrawRectangle(0,0,SCREEN_W,54,{0,0,0,220});
    DrawText("UNIT EDITOR — SANDBOX",14,10,24,C_GOLD);
    DrawText("Modify unit stats (changes persist until restart)",14,38,12,C_SECONDARY);

    int panelY=60, panelH=SCREEN_H-panelY-50;
    int leftW=460, rightX=leftW+14, rightW=SCREEN_W-rightX-8;

    DrawRectangle(8,panelY,leftW,panelH,{10,16,12,210});
    DrawRectangleLinesEx({8,(float)panelY,(float)leftW,(float)panelH},1,{50,80,50,200});
    DrawRectangle(rightX,panelY,rightW,panelH,{10,12,28,210});
    DrawRectangleLinesEx({(float)rightX,(float)panelY,(float)rightW,(float)panelH},1,{50,50,110,200});

    float lx=18, ly=(float)(panelY+10);
    float sliderW=(float)(leftW-80);

    // Dropdown
    DrawText("Unit type:",(int)lx,(int)ly,12,C_SECONDARY); ly+=16;
    if(unitTypeCount()>0){
        const UnitTypeDef& cur=g_unitTypes[g_editTypeIdx];
        Rectangle dropBtn={lx,ly,(float)(leftW-18),24};
        bool dHv=ptInRect(mouse,dropBtn);
        DrawRectangleRec(dropBtn,dHv?Color{28,68,48,255}:Color{16,38,28,255});
        DrawRectangleLinesEx(dropBtn,1,g_editorDropdownOpen?C_GOLD:Color{50,90,60,255});
        DrawRectangle((int)(lx+4),(int)(ly+4),12,12,{cur.r,cur.g,cur.b,255});
        DrawText(cur.name,(int)(lx+20),(int)(ly+4),13,WHITE);
        if(dHv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) g_editorDropdownOpen=!g_editorDropdownOpen;
        if(g_editorDropdownOpen){
            float oy=ly+26;
            for(int i=0;i<unitTypeCount();i++){
                const UnitTypeDef& td=g_unitTypes[i];
                bool sel=(i==g_editTypeIdx);
                Rectangle opt={lx,oy,(float)(leftW-18),22};
                bool ohv=ptInRect(mouse,opt);
                DrawRectangleRec(opt,sel?Color{28,78,40,255}:ohv?Color{20,48,30,255}:Color{12,28,20,240});
                DrawRectangleLinesEx(opt,1,sel?C_GOLD:Color{40,68,45,255});
                DrawRectangle((int)(lx+4),(int)(oy+4),12,12,{td.r,td.g,td.b,255});
                DrawText(td.name,(int)(lx+20),(int)(oy+4),12,sel?C_GOLD:WHITE);
                if(ohv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                    g_editTypeIdx=i; g_editorDropdownOpen=false; g_editorPreviewDirty=true;
                }
                oy+=23;
            }
            if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                float listH=23.f*unitTypeCount();
                if(!ptInRect(mouse,{lx,ly,(float)(leftW-18),26+listH})) g_editorDropdownOpen=false;
            }
            ly+=(float)(23*unitTypeCount())+30;
        } else ly+=30;
    }

    if(g_editTypeIdx<unitTypeCount()){
        UnitTypeDef& td=g_unitTypes[g_editTypeIdx];
        bool changed=false;

        DrawLine((int)lx,(int)ly,(int)(lx+leftW-18),(int)ly,{50,90,50,80}); ly+=8;

        auto SI=[&](int& field,int mn,int mx,const char* lbl,Color col){
            int nv=drawIntSlider({lx,ly,sliderW,14},field,mn,mx,lbl,mouse,col);
            if(nv!=field){field=nv;changed=true;} ly+=30;
        };
        auto SF=[&](float& field,float mn,float mx,const char* lbl,const char* fmt,Color col){
            float nv=drawFloatSlider({lx,ly,sliderW,14},field,mn,mx,lbl,fmt,mouse,col);
            if(nv!=field){field=nv;changed=true;} ly+=30;
        };

        DrawText("STATS",(int)lx,(int)ly,11,{80,200,80,255}); ly+=14;
        SI(td.soldierCount,   1,120,"Soldiers",{80,180,80,200});
        SI(td.armor,          0, 40,"Armor",{160,200,220,200});
        SI(td.speed,         30,200,"Speed",{60,180,220,200});
        SI(td.meleeAttack,    1, 80,"Melee Attack",{220,180,40,200});
        SI(td.meleeDefense,   0, 60,"Melee Defense",{180,140,40,200});
        SF(td.meleeBaseDmg,   1.f, 80.f,"Melee Base Dmg","%.0f",{220,100,40,200});
        SF(td.meleeAPDmg,     0.f, 60.f,"Melee AP Dmg","%.0f",{220,60,40,200});
        SF(td.meleeInterval,0.5f,4.f,"Melee Interval","%.1fs",{180,80,80,200});
        SI(td.range,          0,400,"Range",{200,200,80,200});
        SF(td.missileBaseDmg, 0.f, 60.f,"Missile Base","%.0f",{180,80,200,200});
        SF(td.missileAPDmg,   0.f, 40.f,"Missile AP","%.0f",{160,60,200,200});
        SF(td.missileReload,0.5f,8.f,"Missile Reload","%.1fs",{160,80,180,200});

        DrawLine((int)lx,(int)ly,(int)(lx+leftW-18),(int)ly,{50,90,50,80}); ly+=6;
        DrawText("COLOR",(int)lx,(int)ly,11,{80,200,80,255}); ly+=14;
        int nr=drawIntSlider({lx+16,ly,sliderW,12},(int)td.r,0,255,"R",mouse,{220,60,60,200});
        DrawRectangle((int)lx,(int)ly,12,12,{(unsigned char)nr,0,0,255}); ly+=24;
        int ng=drawIntSlider({lx+16,ly,sliderW,12},(int)td.g,0,255,"G",mouse,{60,220,60,200});
        DrawRectangle((int)lx,(int)ly,12,12,{0,(unsigned char)ng,0,255}); ly+=24;
        int nb=drawIntSlider({lx+16,ly,sliderW,12},(int)td.b,0,255,"B",mouse,{60,60,220,200});
        DrawRectangle((int)lx,(int)ly,12,12,{0,0,(unsigned char)nb,255}); ly+=26;
        if((unsigned char)nr!=td.r||(unsigned char)ng!=td.g||(unsigned char)nb!=td.b){
            td.r=(unsigned char)nr; td.g=(unsigned char)ng; td.b=(unsigned char)nb; changed=true;
        }

        if(changed){ rebuildTexture(g_editTypeIdx); g_editorPreviewDirty=true; }
        if(g_editorPreviewDirty){ rebuildEditorPreview(); g_editorPreviewDirty=false; }

        // Right panel
        float rx=(float)rightX, ry=(float)panelY;
        DrawText(td.name,(int)(rx+10),(int)(ry+10),16,SKYBLUE);
        DrawLine((int)rx,(int)(ry+32),(int)(rx+rightW),(int)(ry+32),{50,50,110,80});

        if(g_editorPrevTex.id>0){
            Vector2 center={rx+rightW/2, ry+120};
            drawSoldierSprite(g_editorPrevTex,center,g_editorPreviewAngle,3.f,
                              {td.r,td.g,td.b,255},false);
            DrawCircleLines((int)center.x,(int)center.y,60,{100,100,200,60});
        }

        float sy=ry+220;
        DrawText("DERIVED STATS",(int)(rx+10),(int)sy,12,{100,180,255,255}); sy+=16;
        DrawText(TextFormat("Total HP: %.0f",(float)td.soldierCount*td.hpPerSoldier),(int)(rx+10),(int)sy,12,{60,220,60,255}); sy+=15;
        DrawText(TextFormat("Total Melee Dmg: %.0f",td.meleeBaseDmg+td.meleeAPDmg),(int)(rx+10),(int)sy,12,{220,160,40,255}); sy+=15;
        DrawText(TextFormat("Total Missile Dmg: %.0f",td.missileBaseDmg+td.missileAPDmg),(int)(rx+10),(int)sy,12,{200,100,200,255}); sy+=15;
        DrawLine((int)rx,(int)sy,(int)(rx+rightW),(int)sy,{50,50,110,80}); sy+=8;
        DrawText("STAT BARS",(int)(rx+10),(int)sy,12,{100,180,255,200}); sy+=14;
        drawStatBars(rx+10,sy,rightW-20,(float)(panelY+panelH-sy-8),td);
    }

    DrawRectangle(0,SCREEN_H-44,SCREEN_W,44,{0,0,0,210});
    if(drawSmBtn({(float)(SCREEN_W-130),(float)(SCREEN_H-38),116,30},"BACK",mouse,
                  {50,25,25,255},{80,40,40,255})){
        if(g_editorPrevTex.id>0){UnloadTexture(g_editorPrevTex);g_editorPrevTex={0};}
        return STATE_MAIN_MENU;
    }
    return STATE_UNIT_EDITOR;
}

// ═══════════════════════════════════════════════════════════════════════════
//  STATE: QUICK BATTLE SETUP
// ═══════════════════════════════════════════════════════════════════════════
static GameState updateDrawQuickBattleSetup(Vector2 mouse){
    int n=unitTypeCount();
    g_quickSetup.playerCounts.resize(n,0);
    g_quickSetup.enemyCounts.resize(n,0);

    ClearBackground({8,10,8,255});
    for(int gx=0;gx<SCREEN_W;gx+=40) DrawLine(gx,0,gx,SCREEN_H,{20,32,20,50});
    for(int gy=0;gy<SCREEN_H;gy+=40) DrawLine(0,gy,SCREEN_W,gy,{20,32,20,50});
    DrawRectangle(0,0,SCREEN_W,54,{0,0,0,220});
    DrawText("QUICK BATTLE SETUP",14,10,26,C_GOLD);
    DrawText("Instant battle without campaign",14,38,12,C_SECONDARY);

    float colW=(SCREEN_W-20)/2.f;
    float listTop=62.f, rowH=40.f;
    int pTotal=0,eTotal=0;
    for(int v:g_quickSetup.playerCounts) pTotal+=v;
    for(int v:g_quickSetup.enemyCounts) eTotal+=v;

    DrawText("YOUR ARMY",(int)14,(int)(listTop+4),13,{80,140,255,255});
    DrawText("ENEMY ARMY",(int)(colW+14),(int)(listTop+4),13,{255,100,100,255});
    DrawLine(0,(int)(listTop+22),SCREEN_W,(int)(listTop+22),{50,70,40,100});
    listTop+=26;

    for(int i=0;i<n&&i<16;i++){
        const UnitTypeDef& td=g_unitTypes[i];
        float ry=listTop+i*rowH;
        if(ry+rowH>SCREEN_H-80) break;

        // Player side
        {
            Color rowbg=(i%2==0)?Color{10,18,28,200}:Color{8,14,22,200};
            DrawRectangle(0,(int)ry,(int)(colW-2),(int)rowH,rowbg);
            DrawRectangle(8,(int)(ry+10),14,14,{td.r,td.g,td.b,255});
            DrawText(td.name,26,(int)(ry+11),12,WHITE);
            float bx2=colW-90;
            bool mhv=CheckCollisionPointRec(mouse,{bx2,ry+8,22,22});
            bool phv=CheckCollisionPointRec(mouse,{bx2+50,ry+8,22,22});
            DrawRectangle((int)bx2,(int)(ry+8),22,22,mhv?Color{80,60,30,255}:Color{40,30,15,255});
            DrawRectangleLinesEx({bx2,ry+8,22,22},1,{100,80,40,255});
            DrawText("-",(int)(bx2+7),(int)(ry+10),15,WHITE);
            if(mhv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)&&g_quickSetup.playerCounts[i]>0)
                g_quickSetup.playerCounts[i]--;
            DrawText(TextFormat("%d",g_quickSetup.playerCounts[i]),(int)(bx2+26),(int)(ry+10),14,WHITE);
            DrawRectangle((int)(bx2+50),(int)(ry+8),22,22,phv?Color{30,80,30,255}:Color{15,40,15,255});
            DrawRectangleLinesEx({bx2+50,ry+8,22,22},1,{40,100,40,255});
            DrawText("+",(int)(bx2+56),(int)(ry+10),15,GREEN);
            if(phv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)&&pTotal<16) g_quickSetup.playerCounts[i]++;
        }
        // Enemy side
        {
            float ex2=colW+2;
            Color rowbg=(i%2==0)?Color{26,8,8,200}:Color{20,6,6,200};
            DrawRectangle((int)ex2,(int)ry,(int)(colW-2),(int)rowH,rowbg);
            unsigned char er=clampU8((int)(td.r*0.3f+150));
            DrawRectangle((int)(ex2+8),(int)(ry+10),14,14,{er,(unsigned char)(td.g/4),(unsigned char)(td.b/4),255});
            DrawText(td.name,(int)(ex2+26),(int)(ry+11),12,WHITE);
            float bxe=ex2+colW-90;
            bool mhv=CheckCollisionPointRec(mouse,{bxe,ry+8,22,22});
            bool phv=CheckCollisionPointRec(mouse,{bxe+50,ry+8,22,22});
            DrawRectangle((int)bxe,(int)(ry+8),22,22,mhv?Color{80,30,30,255}:Color{40,15,15,255});
            DrawRectangleLinesEx({bxe,ry+8,22,22},1,{100,40,40,255});
            DrawText("-",(int)(bxe+7),(int)(ry+10),15,WHITE);
            if(mhv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)&&g_quickSetup.enemyCounts[i]>0)
                g_quickSetup.enemyCounts[i]--;
            DrawText(TextFormat("%d",g_quickSetup.enemyCounts[i]),(int)(bxe+26),(int)(ry+10),14,WHITE);
            DrawRectangle((int)(bxe+50),(int)(ry+8),22,22,phv?Color{80,30,30,255}:Color{40,15,15,255});
            DrawRectangleLinesEx({bxe+50,ry+8,22,22},1,{120,40,40,255});
            DrawText("+",(int)(bxe+56),(int)(ry+10),15,RED);
            if(phv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)&&eTotal<16) g_quickSetup.enemyCounts[i]++;
        }
    }

    DrawLine((int)colW,(int)listTop,(int)colW,SCREEN_H-72,{50,70,40,120});

    int barY=SCREEN_H-72;
    DrawRectangle(0,barY,SCREEN_W,72,{0,0,0,220});
    DrawLine(0,barY,SCREEN_W,barY,{80,65,30,120});
    DrawText(TextFormat("Your: %d groups  |  Enemy: %d groups",pTotal,eTotal),
             14,barY+10,13,C_SECONDARY);
    DrawText(TextFormat("Turn: %d",g_campaign.turn),14,barY+28,13,C_SECONDARY);

    bool canStart=(pTotal>0&&eTotal>0);
    Color cn=canStart?Color{28,65,28,255}:Color{30,30,30,255};
    Color ch=canStart?Color{50,120,50,255}:Color{30,30,30,255};
    if(drawButton({(float)(SCREEN_W-420),(float)(barY+10),280,50},"START BATTLE",mouse,cn,ch)&&canStart){
        std::vector<std::pair<int,int>> pGroups,eGroups;
        for(int i=0;i<n;i++){
            if(g_quickSetup.playerCounts[i]>0) pGroups.push_back({i,g_unitTypes[i].soldierCount});
            if(g_quickSetup.enemyCounts[i]>0) eGroups.push_back({i,g_unitTypes[i].soldierCount});
        }
        initBattle(pGroups,eGroups,TERRAIN_PLAIN,-1,false,"Quick Battle");
        g_quickBattle=true;
        return STATE_BATTLE;
    }
    if(!canStart) DrawText("Add units to both sides",(int)(SCREEN_W-420),barY+62,12,{180,90,40,255});
    if(drawButton({(float)(SCREEN_W-128),(float)(barY+10),114,50},"BACK",mouse,
                  {50,25,25,255},{80,40,40,255})) return STATE_MAIN_MENU;
    return STATE_QUICK_BATTLE_SETUP;
}

// ═══════════════════════════════════════════════════════════════════════════
//  STATE: MARKETPLACE (6.5: Resource trading system)
// ═══════════════════════════════════════════════════════════════════════════
static GameState updateDrawMarketplace(Vector2 mouse){
    ClearBackground(C_BG);
    for(int gx=0;gx<SCREEN_W;gx+=40) DrawLine(gx,0,gx,SCREEN_H,{20,18,14,50});
    for(int gy=0;gy<SCREEN_H;gy+=40) DrawLine(0,gy,SCREEN_W,gy,{20,18,14,50});

    DrawRectangle(0,0,SCREEN_W,50,{0,0,0,220});
    DrawText("MARKETPLACE — Trade Resources",14,10,26,C_GOLD);
    DrawText(TextFormat("Turn: %d  |  Current reserves shown below",g_campaign.turn),14,38,12,C_SECONDARY);

    Resources& res=g_campaign.res;
    static const char* resNames[5]={"Gold","Food","Wood","Stone","Iron"};
    static const Color resColors[5]={{200,165,80,255},{140,100,50,255},{100,150,60,255},
                                       {160,150,140,255},{200,100,50,255}};
    float* resVals[5]={&res.gold,&res.food,&res.wood,&res.stone,&res.iron};

    float tabX=14.f, tabY=60.f, tabW=240.f, tabH=34.f;
    // Tabs for each resource
    for(int i=0;i<5;i++){
        float tx=tabX+i*(tabW+2);
        bool sel=(g_tradeTab==i);
        Color tc=sel?Color{40,80,80,255}:Color{20,40,40,255};
        Color th=sel?Color{60,120,120,255}:Color{35,65,65,255};
        if(drawSmBtn({tx,tabY,tabW,tabH},resNames[i],mouse,tc,th)) g_tradeTab=i;
        if(sel) DrawRectangleLinesEx({tx,tabY,tabW,tabH},2,C_GOLD);
    }

    tabY+=48.f;
    float panelH=(float)(SCREEN_H-100-50);

    // Trading panel for selected resource
    const char* resName=resNames[g_tradeTab];
    Color resCol=resColors[g_tradeTab];
    float* resVal=resVals[g_tradeTab];

    DrawRectangle(8,(int)tabY,SCREEN_W-16,(int)panelH,{12,10,8,200});
    DrawRectangleLinesEx({8,tabY,(float)(SCREEN_W-16),panelH},1,resCol);

    float cx=20.f, cy=tabY+15.f;

    // Current reserves
    DrawText(TextFormat("Current %s: %.0f",resName,*resVal),(int)cx,(int)cy,16,C_PARCHMENT);
    cy+=28;

    // Exchange rate info
    float rate=TRADE_RATES[g_tradeTab];
    DrawText(TextFormat("Exchange rate: 1 %s = %.2f gold (sell)",resName,rate),
             (int)cx,(int)cy,13,C_SECONDARY);
    cy+=22;
    DrawText(TextFormat("Buy: %.2f gold per unit",rate*1.3f),
             (int)cx,(int)cy,13,resCol);
    cy+=28;

    // Amount slider
    g_tradeAmount=drawIntSlider({cx,cy,400.f,14},g_tradeAmount,1,100,"Amount to trade:",mouse,resCol);
    cy+=30;

    float buyPrice=g_tradeAmount*rate*1.3f;
    float sellPrice=g_tradeAmount*rate;

    // Buy button
    bool canBuy=(res.gold>=buyPrice);
    Color buyCol=canBuy?Color{30,80,30,255}:Color{40,20,20,255};
    Color buyColH=canBuy?Color{55,120,55,255}:Color{50,25,25,255};
    if(drawButton({cx,cy,180,44},
                  TextFormat("BUY %d (%.0f g)",g_tradeAmount,buyPrice),
                  mouse,buyCol,buyColH)&&canBuy){
        res.gold-=buyPrice;
        *resVal+=g_tradeAmount;
        battleLogAdd(TextFormat("[Market] Bought %d %s for %.0f gold",g_tradeAmount,resName,buyPrice));
    }

    // Sell button
    bool canSell=(*resVal>=g_tradeAmount);
    Color sellCol=canSell?Color{30,30,80,255}:Color{40,20,20,255};
    Color sellColH=canSell?Color{55,55,120,255}:Color{50,25,25,255};
    if(drawButton({cx+190,cy,180,44},
                  TextFormat("SELL %d (%.0f g)",g_tradeAmount,sellPrice),
                  mouse,sellCol,sellColH)&&canSell){
        *resVal-=g_tradeAmount;
        res.gold+=sellPrice;
        battleLogAdd(TextFormat("[Market] Sold %d %s for %.0f gold",g_tradeAmount,resName,sellPrice));
    }

    cy+=54;

    // All resources summary
    DrawLine(20,(int)(cy+6),(int)(SCREEN_W-20),(int)(cy+6),{50,45,35,100});
    cy+=14;
    DrawText("All Reserves:",(int)cx,(int)cy,13,C_GOLD); cy+=16;
    for(int i=0;i<5;i++){
        DrawText(TextFormat("%s: %.0f",resNames[i],*resVals[i]),(int)(cx+10),(int)(cy+i*16),12,
                 i==g_tradeTab?resColors[i]:Color{100,100,100,255});
    }

    // Bottom buttons
    DrawRectangle(0,SCREEN_H-50,SCREEN_W,50,{0,0,0,210});
    if(drawSmBtn({(float)(SCREEN_W-130),(float)(SCREEN_H-42),116,30},"BACK",mouse,
                  {50,25,25,255},{80,40,40,255}))
        return STATE_CAMPAIGN_MAP;
    return STATE_MARKETPLACE;
}

// ═══════════════════════════════════════════════════════════════════════════
//  STATE: MAIN MENU
// ═══════════════════════════════════════════════════════════════════════════
static GameState updateDrawMainMenu(Vector2 mouse){
    ClearBackground({6,8,6,255});
    // Fog rects
    static float fogScroll=0;
    fogScroll+=20.f*GetFrameTime();
    for(int i=0;i<12;i++){
        float fx=fmodf(fogScroll+i*150.f,(float)SCREEN_W+300)-150;
        float fy=100.f+sinf((float)i*0.7f+fogScroll*0.01f)*50.f;
        DrawRectangle((int)fx,(int)fy,120,(int)(SCREEN_H-fy),{15,20,12,(unsigned char)(8+i*2)});
    }
    // Grid
    for(int x2=0;x2<SCREEN_W;x2+=60) DrawLine(x2,0,x2,SCREEN_H,{22,30,18,40});
    for(int y2=0;y2<SCREEN_H;y2+=60) DrawLine(0,y2,SCREEN_W,y2,{22,30,18,40});
    // Scanlines
    for(int y2=0;y2<SCREEN_H;y2+=3) DrawLine(0,y2,SCREEN_W,y2,{0,0,0,18});

    // Title
    const char* title="MEDIEVAL CONQUEST";
    int tsz=58;
    int ttw=MeasureText(title,tsz);
    DrawText(title,SCREEN_W/2-ttw/2+3,72+3,tsz,{0,60,0,180});
    DrawText(title,SCREEN_W/2-ttw/2,72,tsz,C_GOLD);
    const char* sub="Campaign RTS — Forge an Empire";
    DrawText(sub,SCREEN_W/2-MeasureText(sub,17)/2,142,17,{160,140,80,255});
    DrawLine(SCREEN_W/2-230,168,SCREEN_W/2+230,168,{80,65,30,180});
    DrawLine(SCREEN_W/2-210,172,SCREEN_W/2+210,172,{50,40,20,120});

    float bx=(float)(SCREEN_W/2-175), bw=350.f, bh=50.f;
    if(drawButton({bx,190,bw,bh},"NEW CAMPAIGN",mouse)) {
        newCampaign();
        return STATE_CAMPAIGN_MAP;
    }
    if(drawButton({bx,250,bw,bh},"CONTINUE",mouse,
                  g_hasSave?Color{40,55,40,255}:Color{30,30,30,255},
                  g_hasSave?Color{70,110,60,255}:Color{30,30,30,255})&&g_hasSave){
        if(loadGame()) return STATE_CAMPAIGN_MAP;
    }
    if(drawButton({bx,310,bw,bh},"QUICK BATTLE",mouse)){
        g_quickSetup.playerCounts.assign(unitTypeCount(),0);
        g_quickSetup.enemyCounts.assign(unitTypeCount(),0);
        if(unitTypeCount()>0){ g_quickSetup.playerCounts[0]=2; g_quickSetup.enemyCounts[0]=2; }
        return STATE_QUICK_BATTLE_SETUP;
    }
    if(drawButton({bx,370,bw,bh},"UNIT EDITOR",mouse)){
        g_editorPreviewDirty=true;
        rebuildEditorPreview();
        return STATE_UNIT_EDITOR;
    }
    if(drawButton({bx,430,bw,bh},"SETTINGS",mouse)) return STATE_SETTINGS;
    if(drawButton({bx,490,bw,bh},"EXIT",mouse,{60,28,28,255},{100,45,45,255})){
        g_quitRequested=true;
    }

    const char* hint="LClick/Drag: select  |  RClick: move/attack  |  WASD/Arrows: pan  |  Wheel: zoom  |  F11: fullscreen";
    DrawText(hint,SCREEN_W/2-MeasureText(hint,10)/2,SCREEN_H-18,10,{60,80,50,255});
    return STATE_MAIN_MENU;
}

// ═══════════════════════════════════════════════════════════════════════════
//  SAVE / LOAD — Campaign persistence in dedicated file
// ═══════════════════════════════════════════════════════════════════════════
// (CAMPAIGN_SAVE_FILE and SAVE_VERSION defined at top of file)

static void saveGame(){
    FILE* f=fopen(CAMPAIGN_SAVE_FILE,"wb");
    if(!f) return;
    fwrite(&SAVE_VERSION,sizeof(int),1,f);
    fwrite(&g_campaign.turn,sizeof(int),1,f);
    fwrite(&g_campaign.res,sizeof(Resources),1,f);
    fwrite(&g_campaign.playerProvince,sizeof(int),1,f);
    fwrite(&g_campaign.pendingBattleProvince,sizeof(int),1,f);
    fwrite(&g_campaign.pendingBattleIsDefense,sizeof(bool),1,f);
    fwrite(&g_campaign.viewedCity,sizeof(int),1,f);

    int np=(int)g_campaign.provinces.size();
    fwrite(&np,sizeof(int),1,f);
    for(auto& p:g_campaign.provinces){
        fwrite(p.name,1,32,f);
        fwrite(&p.center.x,sizeof(float),1,f);
        fwrite(&p.center.y,sizeof(float),1,f);
        int ter=(int)p.terrain; fwrite(&ter,sizeof(int),1,f);
        int own=(int)p.owner;   fwrite(&own,sizeof(int),1,f);
        fwrite(&p.hasCity,sizeof(bool),1,f);
        if(p.hasCity){
            fwrite(p.city.name,1,32,f);
            fwrite(p.city.built,sizeof(bool),BLD_COUNT,f);
            fwrite(&p.city.constructing,sizeof(int),1,f);
            fwrite(&p.city.constructTurns,sizeof(int),1,f);
            fwrite(&p.city.defBonus,sizeof(float),1,f);
        }
        int nadj=(int)p.adjacent.size();
        fwrite(&nadj,sizeof(int),1,f);
        for(int a : p.adjacent) fwrite(&a,sizeof(int),1,f);
        int narmy=(int)p.army.size();
        fwrite(&narmy,sizeof(int),1,f);
        for(auto& ap : p.army){ fwrite(&ap.first,sizeof(int),1,f); fwrite(&ap.second,sizeof(int),1,f); }
    }

    int nq=(int)g_campaign.recruitQueue.size();
    fwrite(&nq,sizeof(int),1,f);
    for(auto& e:g_campaign.recruitQueue){
        fwrite(&e.typeIdx,sizeof(int),1,f);
        fwrite(&e.turnsLeft,sizeof(int),1,f);
    }

    int na=(int)g_campaign.playerArmy.size();
    fwrite(&na,sizeof(int),1,f);
    for(auto& a:g_campaign.playerArmy){
        fwrite(&a.first,sizeof(int),1,f);
        fwrite(&a.second,sizeof(int),1,f);
    }

    int nr=(int)g_campaign.readyUnits.size();
    fwrite(&nr,sizeof(int),1,f);
    for(auto& r:g_campaign.readyUnits){
        fwrite(&r.first,sizeof(int),1,f);
        fwrite(&r.second,sizeof(int),1,f);
    }

    fclose(f);
    g_hasSave=true;
}

static bool loadGame(){
    FILE* f=fopen(CAMPAIGN_SAVE_FILE,"rb");
    if(!f) return false;
    int ver=0;
    if(fread(&ver,sizeof(int),1,f)!=1||ver!=SAVE_VERSION){ fclose(f); return false; }

    if(fread(&g_campaign.turn,sizeof(int),1,f)!=1){ fclose(f); return false; }
    if(fread(&g_campaign.res,sizeof(Resources),1,f)!=1){ fclose(f); return false; }
    if(fread(&g_campaign.playerProvince,sizeof(int),1,f)!=1){ fclose(f); return false; }
    if(fread(&g_campaign.pendingBattleProvince,sizeof(int),1,f)!=1){ fclose(f); return false; }
    if(fread(&g_campaign.pendingBattleIsDefense,sizeof(bool),1,f)!=1){ fclose(f); return false; }
    if(fread(&g_campaign.viewedCity,sizeof(int),1,f)!=1){ fclose(f); return false; }

    int np=0;
    if(fread(&np,sizeof(int),1,f)!=1||np<=0||np>256){ fclose(f); return false; }
    g_campaign.provinces.clear();
    g_campaign.provinces.resize(np);
    for(int i=0;i<np;i++){
        Province& p=g_campaign.provinces[i];
        if(fread(p.name,1,32,f)!=32){ fclose(f); return false; }
        if(fread(&p.center.x,sizeof(float),1,f)!=1){ fclose(f); return false; }
        if(fread(&p.center.y,sizeof(float),1,f)!=1){ fclose(f); return false; }
        int ter=0,own=0;
        if(fread(&ter,sizeof(int),1,f)!=1){ fclose(f); return false; }
        if(fread(&own,sizeof(int),1,f)!=1){ fclose(f); return false; }
        p.terrain=(TerrainType)ter;
        p.owner=(FactionId)own;
        if(fread(&p.hasCity,sizeof(bool),1,f)!=1){ fclose(f); return false; }
        if(p.hasCity){
            if(fread(p.city.name,1,32,f)!=32){ fclose(f); return false; }
            if(fread(p.city.built,sizeof(bool),BLD_COUNT,f)!=BLD_COUNT){ fclose(f); return false; }
            if(fread(&p.city.constructing,sizeof(int),1,f)!=1){ fclose(f); return false; }
            if(fread(&p.city.constructTurns,sizeof(int),1,f)!=1){ fclose(f); return false; }
            if(fread(&p.city.defBonus,sizeof(float),1,f)!=1){ fclose(f); return false; }
        }
        int nadj=0;
        if(fread(&nadj,sizeof(int),1,f)!=1||nadj<0||nadj>64){ fclose(f); return false; }
        p.adjacent.clear();
        for(int j=0;j<nadj;j++){ int a=0; if(fread(&a,sizeof(int),1,f)!=1){ fclose(f); return false; } p.adjacent.push_back(a); }
        int narmy=0;
        if(fread(&narmy,sizeof(int),1,f)!=1||narmy<0||narmy>128){ fclose(f); return false; }
        p.army.clear();
        for(int j=0;j<narmy;j++){
            int ti=0,cnt=0;
            if(fread(&ti,sizeof(int),1,f)!=1||fread(&cnt,sizeof(int),1,f)!=1){ fclose(f); return false; }
            p.army.push_back({ti,cnt});
        }
    }

    int nq=0;
    if(fread(&nq,sizeof(int),1,f)!=1||nq<0||nq>256){ fclose(f); return false; }
    g_campaign.recruitQueue.clear();
    for(int i=0;i<nq;i++){
        int ti=0,tl=0;
        if(fread(&ti,sizeof(int),1,f)!=1||fread(&tl,sizeof(int),1,f)!=1){ fclose(f); return false; }
        g_campaign.recruitQueue.push_back({ti,tl});
    }

    int na=0;
    if(fread(&na,sizeof(int),1,f)!=1||na<0||na>256){ fclose(f); return false; }
    g_campaign.playerArmy.clear();
    for(int i=0;i<na;i++){
        int ti=0,cnt=0;
        if(fread(&ti,sizeof(int),1,f)!=1||fread(&cnt,sizeof(int),1,f)!=1){ fclose(f); return false; }
        g_campaign.playerArmy.push_back({ti,cnt});
    }

    int nr=0;
    if(fread(&nr,sizeof(int),1,f)!=1||nr<0||nr>256){ fclose(f); return false; }
    g_campaign.readyUnits.clear();
    for(int i=0;i<nr;i++){
        int ti=0,cnt=0;
        if(fread(&ti,sizeof(int),1,f)!=1||fread(&cnt,sizeof(int),1,f)!=1){ fclose(f); return false; }
        g_campaign.readyUnits.push_back({ti,cnt});
    }

    fclose(f);
    updateProvinceCenters();
    g_campaign.playerArmy = g_campaign.readyUnits; // 1.4: sync after load
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
//  STATE: VICTORY
// ═══════════════════════════════════════════════════════════════════════════
static GameState updateDrawVictory(Vector2 mouse){
    ClearBackground({5,10,5,255});
    DrawRectangleGradientV(0,0,SCREEN_W,SCREEN_H,{0,40,10,255},{5,10,5,255});
    // Title
    const char* ttl="VICTORY!";
    int tsz=72;
    int ttw=MeasureText(ttl,tsz);
    DrawText(ttl,SCREEN_W/2-ttw/2+4,60+4,tsz,{0,60,0,160});
    DrawText(ttl,SCREEN_W/2-ttw/2,60,tsz,{80,220,80,255});
    DrawLine(SCREEN_W/2-250,145,SCREEN_W/2+250,145,C_GOLD);

    DrawText("Your empire now dominates the realm!",
             SCREEN_W/2-MeasureText("Your empire now dominates the realm!",18)/2,
             165,18,C_PARCHMENT);

    // Stats
    float sy=210.f;
    DrawText(TextFormat("Final Turn: %d",g_campaign.turn),(int)(SCREEN_W/2-200),(int)sy,15,C_SECONDARY); sy+=24;
    DrawText(TextFormat("Battles Won: %d",g_campaign.battlesWon),(int)(SCREEN_W/2-200),(int)sy,15,C_PARCHMENT); sy+=24;
    DrawText(TextFormat("Battles Lost: %d",g_campaign.battlesLost),(int)(SCREEN_W/2-200),(int)sy,15,C_SECONDARY); sy+=24;
    DrawText(TextFormat("Peak Provinces Held: %d",g_campaign.peakProvinces),(int)(SCREEN_W/2-200),(int)sy,15,C_GOLD); sy+=40;

    if(drawButton({(float)(SCREEN_W/2-180),(float)sy,360,54},"NEW CAMPAIGN",mouse,{30,65,30,255},{55,110,50,255})){
        newCampaign();
        return STATE_CAMPAIGN_MAP;
    }
    if(drawButton({(float)(SCREEN_W/2-180),(float)(sy+64),360,50},"MAIN MENU",mouse,{50,25,25,255},{80,40,40,255}))
        return STATE_MAIN_MENU;
    return STATE_VICTORY;
}

// ═══════════════════════════════════════════════════════════════════════════
//  STATE: DEFEAT
// ═══════════════════════════════════════════════════════════════════════════
static GameState updateDrawDefeat(Vector2 mouse){
    ClearBackground({10,5,5,255});
    DrawRectangleGradientV(0,0,SCREEN_W,SCREEN_H,{40,0,0,255},{10,5,5,255});
    const char* ttl="DEFEAT";
    int tsz=72;
    int ttw=MeasureText(ttl,tsz);
    DrawText(ttl,SCREEN_W/2-ttw/2+4,60+4,tsz,{60,0,0,160});
    DrawText(ttl,SCREEN_W/2-ttw/2,60,tsz,{220,60,60,255});
    DrawLine(SCREEN_W/2-250,145,SCREEN_W/2+250,145,C_COPPER);

    DrawText("Your kingdom has fallen. The realm mourns.",
             SCREEN_W/2-MeasureText("Your kingdom has fallen. The realm mourns.",16)/2,
             165,16,C_SECONDARY);

    float sy=210.f;
    DrawText(TextFormat("Final Turn: %d",g_campaign.turn),(int)(SCREEN_W/2-200),(int)sy,15,C_SECONDARY); sy+=24;
    DrawText(TextFormat("Battles Won: %d",g_campaign.battlesWon),(int)(SCREEN_W/2-200),(int)sy,15,C_PARCHMENT); sy+=24;
    DrawText(TextFormat("Battles Lost: %d",g_campaign.battlesLost),(int)(SCREEN_W/2-200),(int)sy,15,C_ENEMY_COL); sy+=24;
    DrawText(TextFormat("Peak Provinces Held: %d",g_campaign.peakProvinces),(int)(SCREEN_W/2-200),(int)sy,15,C_SECONDARY); sy+=40;

    if(drawButton({(float)(SCREEN_W/2-180),(float)sy,360,54},"TRY AGAIN",mouse,{55,20,20,255},{90,35,35,255})){
        newCampaign();
        return STATE_CAMPAIGN_MAP;
    }
    if(drawButton({(float)(SCREEN_W/2-180),(float)(sy+64),360,50},"MAIN MENU",mouse,{50,25,25,255},{80,40,40,255}))
        return STATE_MAIN_MENU;
    return STATE_DEFEAT;
}

// ═══════════════════════════════════════════════════════════════════════════
//  MAIN
// ═══════════════════════════════════════════════════════════════════════════
int main(){
    InitWindow(SCREEN_W,SCREEN_H,"Medieval Conquest");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    srand((unsigned)time(nullptr)); // 1.2: seed for any remaining rand() calls
    loadSettings();                  // 3.1: load persistent settings
    g_battleLog.clear();

    initBuiltinTypes();
    rebuildEditorPreview();

    // Check for campaign save
    {FILE* tf=fopen(CAMPAIGN_SAVE_FILE,"rb"); if(tf){g_hasSave=true;fclose(tf);}}

    g_state=STATE_MAIN_MENU;

    // Quick battle defaults
    g_quickSetup.playerCounts.assign(unitTypeCount(),0);
    g_quickSetup.enemyCounts.assign(unitTypeCount(),0);
    if(unitTypeCount()>0){ g_quickSetup.playerCounts[0]=2; g_quickSetup.enemyCounts[0]=2; }

    // Pre-battle defaults
    generateCampaignMap();

    while(!WindowShouldClose()&&!g_quitRequested){
        // Fullscreen toggle
        if(IsKeyPressed(KEY_F11)||(IsKeyDown(KEY_LEFT_ALT)&&IsKeyPressed(KEY_ENTER)))
            ToggleFullscreen();

        SCREEN_W=GetScreenWidth();
        SCREEN_H=GetScreenHeight();

        float dt=GetFrameTime();
        if(dt>0.1f) dt=0.1f; // cap delta time
        g_menuTime+=dt;

        Vector2 mouse=GetMousePosition();

        BeginDrawing();
        switch(g_state){
            case STATE_MAIN_MENU:
                g_state=updateDrawMainMenu(mouse);
                break;
            case STATE_CAMPAIGN_MAP:
                g_state=updateDrawCampaignMap(mouse,dt);
                break;
            case STATE_CITY_MANAGEMENT:
                g_state=updateDrawCityManagement(mouse);
                break;
            case STATE_RECRUITMENT:
                g_state=updateDrawRecruitment(mouse);
                break;
            case STATE_PRE_BATTLE:
                g_state=updateDrawPreBattle(mouse);
                break;
            case STATE_BATTLE:
                g_state=updateDrawBattle(mouse,dt);
                break;
            case STATE_BATTLE_RESULT:
                g_state=updateDrawBattleResult(mouse);
                break;
            case STATE_UNIT_CODEX:
                g_state=updateDrawUnitCodex(mouse,dt);
                break;
            case STATE_SETTINGS:
                g_state=updateDrawSettings(mouse);
                break;
            case STATE_UNIT_EDITOR:
                g_state=updateDrawUnitEditor(mouse,dt);
                break;
            case STATE_QUICK_BATTLE_SETUP:
                g_state=updateDrawQuickBattleSetup(mouse);
                break;
            case STATE_VICTORY:
                g_state=updateDrawVictory(mouse);
                break;
            case STATE_DEFEAT:
                g_state=updateDrawDefeat(mouse);
                break;
            case STATE_MARKETPLACE:
                g_state=updateDrawMarketplace(mouse);
                break;
        }
        // 4.9: Global FPS (shown in non-battle states too)
        if(g_settings.showFPS&&g_state!=STATE_BATTLE) DrawFPS(8,8);

        // 4.10: Custom medieval cursor (cross-hair style)
        HideCursor();
        Vector2 cur=GetMousePosition();
        Color curCol=C_GOLD;
        if(g_state==STATE_BATTLE||g_state==STATE_PRE_BATTLE) curCol=C_ALLY;
        else if(g_state==STATE_CAMPAIGN_MAP) curCol={80,220,80,255};
        int csz=6;
        DrawLine((int)cur.x-csz,(int)cur.y,(int)cur.x+csz,(int)cur.y,curCol);
        DrawLine((int)cur.x,(int)cur.y-csz,(int)cur.x,(int)cur.y+csz,curCol);
        DrawRectangleLines((int)cur.x-2,(int)cur.y-2,4,4,{curCol.r,curCol.g,curCol.b,180});

        EndDrawing();

        // 4.10: Custom medieval cursor drawn after EndDrawing is handled by Raylib's software cursor
    }

    ShowCursor();  // 4.10: restore cursor on exit

    // Cleanup textures
    for(auto& t:g_playerTextures) UnloadTexture(t);
    for(auto& t:g_enemyTextures) UnloadTexture(t);
    if(g_editorPrevTex.id>0) UnloadTexture(g_editorPrevTex);

    // Auto-save on exit if campaign active
    if(g_campaign.turn>1) saveGame();

    CloseWindow();
    return 0;
}
