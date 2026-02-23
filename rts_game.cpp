// ═══════════════════════════════════════════════════════════════
//  MEDIEVAL WARFARE — RTS Pixel Art Battle Game
//  Single-file C++ using Raylib
//  Compile: g++ -std=c++17 rts_game.cpp -lraylib -o game
// ═══════════════════════════════════════════════════════════════
#include "raylib.h"
#include <vector>
#include <cmath>
#include <string>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cassert>

// ───────────────────────────────────────────────────────────────
//  SCREEN / GLOBAL CONSTANTS
// ───────────────────────────────────────────────────────────────
int SCREEN_W = 1280;
int SCREEN_H = 768;
static const int BATTLEFIELD_H = 680; // bottom 88px = HUD
static const int HUD_Y         = 680;
static const int SPR_SIZE      = 16;

// ───────────────────────────────────────────────────────────────
//  UTILITY
// ───────────────────────────────────────────────────────────────
unsigned char clampU8(int v){ return (unsigned char)std::max(0,std::min(255,v)); }

static inline float vdist(Vector2 a,Vector2 b){
    float dx=a.x-b.x,dy=a.y-b.y; return sqrtf(dx*dx+dy*dy);
}
static inline Vector2 vnorm(Vector2 v){
    float l=sqrtf(v.x*v.x+v.y*v.y);
    if(l<0.0001f) return {0,0};
    return {v.x/l,v.y/l};
}
static inline float dirToAngle(Vector2 d){ return atan2f(-d.y,d.x)*RAD2DEG; }
static inline float lerpAngle(float c,float t,float s){
    float d=t-c;
    while(d>180) d-=360; while(d<-180) d+=360;
    return c+d*s;
}
static inline float frand(){ return (float)rand()/(float)RAND_MAX; }
static inline bool ptInRect(Vector2 p,Rectangle r){
    return p.x>=r.x&&p.x<=r.x+r.width&&p.y>=r.y&&p.y<=r.y+r.height;
}

// ───────────────────────────────────────────────────────────────
//  SPRITE BASE TYPES
// ───────────────────────────────────────────────────────────────
enum SpriteBase { SPR_INFANTRY=0, SPR_CAVALRY, SPR_RANGED, SPR_BASE_COUNT };
static const char* spriteBaseNames[SPR_BASE_COUNT] = { "Infantry", "Cavalry", "Ranged" };

// pixel drawing helper
static void px(Image& img,int x,int y,Color c){
    if(x<0||y<0||x>=img.width||y>=img.height) return;
    ImageDrawPixel(&img,x,y,c);
}
static void fillRect(Image& img,int x,int y,int w,int h,Color c){
    for(int yy=y;yy<y+h;yy++) for(int xx=x;xx<x+w;xx++) px(img,xx,yy,c);
}

// Make infantry sprite (spear/axe/poleaxe/hammer depending on weapon hint)
Image makeInfantryImg(unsigned char r,unsigned char g,unsigned char b,int weaponHint=0){
    Image img=GenImageColor(SPR_SIZE,SPR_SIZE,BLANK);
    Color body={r,g,b,255};
    Color dark={clampU8(r/2),clampU8(g/2),clampU8(b/2),255};
    Color hi={clampU8(r+70),clampU8(g+70),clampU8(b+70),255};
    Color skin={220,170,120,255};
    Color wpn={200,200,180,255};
    // Legs
    for(int y2=11;y2<=15;y2++){px(img,5,y2,dark);px(img,6,y2,dark);px(img,9,y2,dark);px(img,10,y2,dark);}
    // Body
    fillRect(img,4,6,8,6,body);
    px(img,4,6,hi);px(img,5,6,hi);
    // Head
    fillRect(img,5,2,6,4,skin);
    for(int x2=4;x2<=11;x2++) px(img,x2,2,dark); // helmet top
    px(img,4,3,dark);px(img,11,3,dark);
    // Weapon
    if(weaponHint==0){ // spear
        for(int y2=0;y2<=5;y2++){px(img,7,y2,wpn);px(img,8,y2,wpn);}
        px(img,7,0,YELLOW);px(img,8,0,YELLOW); // tip
    } else if(weaponHint==1){ // axe
        fillRect(img,6,0,5,3,wpn);
        px(img,8,3,wpn);px(img,8,4,wpn);px(img,8,5,wpn);
        px(img,6,0,{180,100,40,255});px(img,10,0,{180,100,40,255});
    } else if(weaponHint==2){ // poleaxe
        for(int y2=0;y2<=6;y2++){px(img,8,y2,wpn);}
        fillRect(img,6,0,4,2,wpn);
        px(img,6,2,{180,180,50,255});
    } else { // hammer
        px(img,8,4,wpn);px(img,8,5,wpn);
        fillRect(img,6,2,5,3,{160,160,180,255});
    }
    return img;
}

Image makeCavalryImg(unsigned char r,unsigned char g,unsigned char b){
    Image img=GenImageColor(SPR_SIZE,SPR_SIZE,BLANK);
    Color body={r,g,b,255};
    Color dark={clampU8(r/2),clampU8(g/2),clampU8(b/2),255};
    Color hi={clampU8(r+70),clampU8(g+70),clampU8(b+70),255};
    Color horse={120,80,40,255};
    Color hdark={70,45,20,255};
    Color wpn={200,200,180,255};
    // Horse body
    fillRect(img,2,8,12,6,horse);
    px(img,1,9,horse);px(img,14,9,horse);
    px(img,2,8,hi);
    // Horse legs
    for(int y2=14;y2<=15;y2++){
        px(img,3,y2,hdark);px(img,5,y2,hdark);
        px(img,10,y2,hdark);px(img,12,y2,hdark);
    }
    // Rider body
    fillRect(img,5,4,6,5,body);
    px(img,5,4,hi);
    // Head
    fillRect(img,6,1,4,3,{220,170,120,255});
    px(img,5,1,dark);px(img,10,1,dark); // helmet sides
    for(int x2=5;x2<=10;x2++) px(img,x2,1,dark); // helmet top
    // Lance
    for(int y2=0;y2<=5;y2++){px(img,8,y2,wpn);}
    px(img,8,0,YELLOW);
    return img;
}

Image makeRangedImg(unsigned char r,unsigned char g,unsigned char b,bool crossbow=false){
    Image img=GenImageColor(SPR_SIZE,SPR_SIZE,BLANK);
    Color body={r,g,b,255};
    Color dark={clampU8(r/2),clampU8(g/2),clampU8(b/2),255};
    Color hi={clampU8(r+70),clampU8(g+70),clampU8(b+70),255};
    Color skin={220,170,120,255};
    Color wpn={160,120,60,255};
    // Legs
    for(int y2=11;y2<=15;y2++){px(img,5,y2,dark);px(img,6,y2,dark);px(img,9,y2,dark);px(img,10,y2,dark);}
    // Body
    fillRect(img,4,6,8,5,body);
    px(img,4,6,hi);px(img,5,6,hi);
    // Head
    fillRect(img,5,2,6,4,skin);
    for(int x2=4;x2<=11;x2++) px(img,x2,2,dark);
    // Bow or crossbow
    if(!crossbow){
        // bow: arc
        px(img,8,0,wpn);px(img,9,1,wpn);px(img,10,2,wpn);px(img,10,3,wpn);
        px(img,10,4,wpn);px(img,9,5,wpn);px(img,8,6,wpn);
        // string
        px(img,8,0,{200,200,200,255});px(img,8,6,{200,200,200,255});
    } else {
        // crossbow: horizontal stock + vertical prod
        fillRect(img,6,4,5,2,wpn);
        fillRect(img,5,3,7,1,{100,80,30,255}); // prod
        px(img,8,4,{150,150,150,255}); // trigger area
    }
    return img;
}

Texture2D imgToTex(Image img){
    Texture2D tex=LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(tex,TEXTURE_FILTER_POINT);
    return tex;
}

// ───────────────────────────────────────────────────────────────
//  UNIT TYPE DEFINITION
// ───────────────────────────────────────────────────────────────
struct UnitTypeDef {
    char name[32];
    int  entities;
    int  hpPerEntity;
    int  armor;
    int  speed;
    int  meleeAttack;
    int  meleeDefense;
    int  meleeBaseDmg;
    int  meleeAPDmg;
    float meleeInterval;  // seconds
    int  range;           // 0 = no ranged
    int  missileBaseDmg;
    int  missileAPDmg;
    float missileReload;  // seconds
    int  cost;
    unsigned char r,g,b;
    SpriteBase spriteBase;
    bool isBuiltin;
    // weapon hint for infantry sprite: 0=spear,1=axe,2=poleaxe,3=hammer
    int weaponHint;
};

static std::vector<UnitTypeDef> g_unitTypes;
static std::vector<Texture2D>   g_playerTextures;
static std::vector<Texture2D>   g_enemyTextures;

void rebuildTexture(int idx){
    const UnitTypeDef& td=g_unitTypes[idx];
    auto makeImg=[&](unsigned char r,unsigned char g,unsigned char b)->Image{
        switch(td.spriteBase){
            case SPR_CAVALRY: return makeCavalryImg(r,g,b);
            case SPR_RANGED:  return makeRangedImg(r,g,b,(td.range>0&&td.meleeInterval>2.0f));
            default:          return makeInfantryImg(r,g,b,td.weaponHint);
        }
    };
    auto setOrPush=[&](std::vector<Texture2D>& vec,Texture2D tex,int i){
        if(i<(int)vec.size()){ UnloadTexture(vec[i]); vec[i]=tex; }
        else vec.push_back(tex);
    };
    setOrPush(g_playerTextures, imgToTex(makeImg(td.r,td.g,td.b)), idx);
    // Enemy: reddish tint
    unsigned char er=clampU8((int)(td.r*0.4f+180));
    unsigned char eg=clampU8((int)(td.g*0.25f));
    unsigned char eb=clampU8((int)(td.b*0.25f));
    setOrPush(g_enemyTextures, imgToTex(makeImg(er,eg,eb)), idx);
}

void initBuiltinTypes(){
    g_unitTypes.clear();
    auto add=[&](const char* name,int ent,int hp,int arm,int spd,
                 int matk,int mdef,int mbase,int map,float mint,
                 int rng,int msbase,int msap,float mrel,int cost,
                 unsigned char r,unsigned char g,unsigned char b,
                 SpriteBase spr,int wh){
        UnitTypeDef t{};
        strncpy(t.name,name,31);
        t.entities=ent; t.hpPerEntity=hp; t.armor=arm; t.speed=spd;
        t.meleeAttack=matk; t.meleeDefense=mdef;
        t.meleeBaseDmg=mbase; t.meleeAPDmg=map; t.meleeInterval=mint;
        t.range=rng; t.missileBaseDmg=msbase; t.missileAPDmg=msap; t.missileReload=mrel;
        t.cost=cost; t.r=r; t.g=g; t.b=b; t.spriteBase=spr; t.weaponHint=wh;
        t.isBuiltin=true;
        g_unitTypes.push_back(t);
    };
    //                  name               ent  hp  arm spd matk mdef mbase map mint rng ms ma mrl cost  r    g    b      spr          wh
    add("Spear Levy",    60, 55,  0,  80, 18, 12, 10,  2, 1.8f,  0,  0,  0, 0.0f,  60, 200,180,140, SPR_INFANTRY, 0);
    add("Axe Militia",   60, 65,  5,  75, 24, 16, 16,  4, 1.6f,  0,  0,  0, 0.0f,  80, 160,110, 60, SPR_INFANTRY, 1);
    add("Poleaxe Ret.",  40, 90, 14,  70, 36, 24, 22,  8, 1.4f,  0,  0,  0, 0.0f, 150, 140,150,160, SPR_INFANTRY, 2);
    add("Dis. Knights",  30,130, 22,  60, 44, 30, 26, 14, 1.6f,  0,  0,  0, 0.0f, 240, 200,210,220, SPR_INFANTRY, 3);
    add("Bow Levy",      60, 50,  0,  85, 16, 10,  8,  1, 2.0f,260, 12,  2, 2.5f,  70, 190,170,100, SPR_RANGED,   0);
    add("Crossbow Ret.", 40, 75,  8,  70, 22, 18, 11,  2, 2.2f,300, 18,  8, 4.0f, 160,  80,130, 80, SPR_RANGED,   0);
    add("Mount. Sgts.",  40, 85,  8, 160, 28, 20, 18,  5, 1.4f,  0,  0,  0, 0.0f, 130, 140, 90, 60, SPR_CAVALRY,  0);
    add("Knights",       24,160, 26, 140, 50, 34, 30, 20, 1.5f,  0,  0,  0, 0.0f, 300, 220,220,200, SPR_CAVALRY,  0);

    g_playerTextures.clear();
    g_enemyTextures.clear();
    for(int i=0;i<(int)g_unitTypes.size();i++) rebuildTexture(i);
}

int unitTypeCount(){ return (int)g_unitTypes.size(); }

// ───────────────────────────────────────────────────────────────
//  GAME STATES
// ───────────────────────────────────────────────────────────────
enum GameState { STATE_MENU, STATE_CUSTOM_BATTLE, STATE_UNIT_EDITOR, STATE_PLAYING };

// ───────────────────────────────────────────────────────────────
//  BATTLE CONFIG
// ───────────────────────────────────────────────────────────────
enum Difficulty { DIFF_EASY=0, DIFF_NORMAL, DIFF_HARD };

struct BattleConfig {
    std::vector<int> playerCounts;
    std::vector<int> enemyCounts;
    Difficulty difficulty;
};

void resizeBattleConfig(BattleConfig& c){
    int n=unitTypeCount();
    c.playerCounts.resize(n,0);
    c.enemyCounts.resize(n,0);
}

BattleConfig defaultBattleConfig(){
    BattleConfig c;
    resizeBattleConfig(c);
    c.difficulty=DIFF_NORMAL;
    if(unitTypeCount()>0) c.playerCounts[0]=3;
    if(unitTypeCount()>0) c.enemyCounts[0]=3;
    return c;
}

// ───────────────────────────────────────────────────────────────
//  IN-BATTLE UNIT STRUCT
// ───────────────────────────────────────────────────────────────
enum UnitAction { ACT_IDLE, ACT_MOVING, ACT_ATTACKING, ACT_ROUTING };

struct BUnit {
    Vector2  pos, target;
    int      orderTarget;    // index into army of explicit attack target (-1 = none)
    float    hp, maxHp;
    float    angle, angleTarget;
    float    meleeTimer, rangeTimer;
    float    stopTimer;      // for cavalry charge reset
    bool     alive, selected, hasOrder, moving;
    UnitAction action;
    int      typeIdx;
    bool     isPlayer;
    bool     chargeReady;    // cavalry charge bonus available
    bool     inMelee;        // currently in melee range of someone
};

struct DeadMarker {
    Vector2 pos;
    float   alpha;
};

struct Projectile {
    Vector2 pos, vel;
    float   damage;
    bool    alive, fromPlayer;
};

// ───────────────────────────────────────────────────────────────
//  UI HELPERS
// ───────────────────────────────────────────────────────────────
bool drawButton(Rectangle r,const char* lbl,Vector2 m,
                Color cn={40,60,40,255},Color ch={70,120,60,255}){
    bool hv=ptInRect(m,r);
    DrawRectangleRec(r,hv?ch:cn);
    DrawRectangleLinesEx(r,2,hv?Color{180,220,180,255}:Color{80,130,80,255});
    int tw=MeasureText(lbl,18);
    DrawText(lbl,(int)(r.x+r.width/2-tw/2),(int)(r.y+r.height/2-9),18,hv?WHITE:LIGHTGRAY);
    return hv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

bool drawSmallButton(Rectangle r,const char* lbl,Vector2 m,
                     Color cn={30,50,30,255},Color ch={60,100,50,255}){
    bool hv=ptInRect(m,r);
    DrawRectangleRec(r,hv?ch:cn);
    DrawRectangleLinesEx(r,1,hv?GREEN:Color{60,100,60,255});
    int tw=MeasureText(lbl,14);
    DrawText(lbl,(int)(r.x+r.width/2-tw/2),(int)(r.y+r.height/2-7),14,hv?WHITE:LIGHTGRAY);
    return hv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

// integer slider (returns new value, draws inline)
int drawIntSlider(Rectangle r,int val,int vmin,int vmax,
                  const char* label,Vector2 mouse,Color fill={60,180,60,200}){
    if(label&&label[0]){
        DrawText(label,(int)r.x,(int)(r.y-16),13,LIGHTGRAY);
    }
    DrawRectangleRec(r,{20,20,20,255});
    DrawRectangleLinesEx(r,1,{60,60,60,255});
    float t=(float)(val-vmin)/(float)(vmax-vmin);
    DrawRectangle((int)r.x,(int)r.y,(int)(t*r.width),(int)r.height,fill);
    float kx=r.x+t*r.width;
    DrawRectangle((int)(kx-4),(int)(r.y-2),8,(int)(r.height+4),WHITE);
    DrawText(TextFormat("%d",val),(int)(r.x+r.width+6),(int)(r.y+1),13,WHITE);
    if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)&&ptInRect(mouse,r)){
        float nt=(mouse.x-r.x)/r.width;
        nt=fmaxf(0,fminf(1,nt));
        val=vmin+(int)roundf(nt*(float)(vmax-vmin));
    }
    return val;
}

float drawFloatSlider(Rectangle r,float val,float vmin,float vmax,
                      const char* label,const char* fmt,Vector2 mouse,Color fill={60,180,60,200}){
    if(label&&label[0]) DrawText(label,(int)r.x,(int)(r.y-16),13,LIGHTGRAY);
    DrawRectangleRec(r,{20,20,20,255});
    DrawRectangleLinesEx(r,1,{60,60,60,255});
    float t=(val-vmin)/(vmax-vmin);
    DrawRectangle((int)r.x,(int)r.y,(int)(t*r.width),(int)r.height,fill);
    float kx=r.x+t*r.width;
    DrawRectangle((int)(kx-4),(int)(r.y-2),8,(int)(r.height+4),WHITE);
    DrawText(TextFormat(fmt,val),(int)(r.x+r.width+6),(int)(r.y+1),13,WHITE);
    if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)&&ptInRect(mouse,r)){
        float nt=(mouse.x-r.x)/r.width;
        nt=fmaxf(0,fminf(1,nt));
        val=vmin+nt*(vmax-vmin);
    }
    return val;
}

void drawHealthBar(Vector2 pos,float hp,float maxHp,float w){
    float ratio=hp/maxHp;
    DrawRectangle((int)(pos.x-w/2),(int)(pos.y-24),(int)w,5,DARKGRAY);
    Color c=ratio>0.5f?GREEN:ratio>0.25f?YELLOW:RED;
    DrawRectangle((int)(pos.x-w/2),(int)(pos.y-24),(int)(w*ratio),5,c);
}

void drawSprite(Texture2D tex,Vector2 pos,float angleDeg,float scale,Color tint){
    float s=SPR_SIZE*scale;
    DrawTexturePro(tex,{0,0,(float)SPR_SIZE,(float)SPR_SIZE},
                   {pos.x,pos.y,s,s},{s/2,s/2},-angleDeg+90.0f,tint);
}

// ═══════════════════════════════════════════════════════════════
//  MAIN MENU
// ═══════════════════════════════════════════════════════════════
// Returns next state; caller checks for QUIT via a flag
static bool g_quitRequested=false;

GameState updateDrawMenu(Vector2 mouse,float t){
    ClearBackground({8,10,8,255});

    // Animated grid
    for(int x=0;x<SCREEN_W;x+=48){
        float alpha=60+20*(int)sinf(t*0.7f+x*0.02f);
        DrawLine(x,0,x,SCREEN_H,{25,50,25,(unsigned char)alpha});
    }
    for(int y=0;y<SCREEN_H;y+=48){
        float alpha=60+20*(int)sinf(t*0.5f+y*0.02f);
        DrawLine(0,y,SCREEN_W,y,{25,50,25,(unsigned char)alpha});
    }
    // Scanlines
    for(int y=0;y<SCREEN_H;y+=3)
        DrawLine(0,y,SCREEN_W,y,{0,0,0,30});

    // Title
    const char* title="MEDIEVAL WARFARE";
    int tsz=54;
    int tw=MeasureText(title,tsz);
    DrawText(title,SCREEN_W/2-tw/2+3,83,tsz,{0,60,0,200});
    DrawText(title,SCREEN_W/2-tw/2,  80,tsz,GREEN);
    const char* sub="Top-down RTS Battle Simulator";
    DrawText(sub,SCREEN_W/2-MeasureText(sub,18)/2,148,18,{100,180,100,255});

    // Decorative lines under title
    DrawLine(SCREEN_W/2-220,178,SCREEN_W/2+220,178,{60,120,60,200});
    DrawLine(SCREEN_W/2-200,182,SCREEN_W/2+200,182,{40,80,40,150});

    float bx=(float)(SCREEN_W/2-160), bw=320.0f, bh=52.0f;
    bool cust=drawButton({bx,220,bw,bh},"CUSTOM BATTLE",mouse);
    bool edit=drawButton({bx,290,bw,bh},"UNIT EDITOR",mouse);
    bool quit=drawButton({bx,360,bw,bh},"EXIT GAME",mouse,{60,30,30,255},{110,50,50,255});

    if(quit) g_quitRequested=true;
    if(cust) return STATE_CUSTOM_BATTLE;
    if(edit) return STATE_UNIT_EDITOR;

    // Controls hint
    const char* hint="LClick: select  |  Drag: box select  |  RClick: move/attack  |  ESC: menu  |  F11: fullscreen";
    DrawText(hint,SCREEN_W/2-MeasureText(hint,11)/2,SCREEN_H-22,11,{60,100,60,255});

    return STATE_MENU;
}

// ═══════════════════════════════════════════════════════════════
//  CUSTOM BATTLE
// ═══════════════════════════════════════════════════════════════
GameState updateDrawCustomBattle(BattleConfig& cfg,Vector2 mouse){
    resizeBattleConfig(cfg);

    ClearBackground({8,10,8,255});
    for(int gx=0;gx<SCREEN_W;gx+=40) DrawLine(gx,0,gx,SCREEN_H,{20,32,20,50});
    for(int gy=0;gy<SCREEN_H;gy+=40) DrawLine(0,gy,SCREEN_W,gy,{20,32,20,50});

    DrawRectangle(0,0,SCREEN_W,54,{0,0,0,220});
    DrawText("CUSTOM BATTLE",16,10,28,GREEN);
    DrawText("Set up both armies and choose difficulty",16,40,12,{100,160,100,255});

    // Column layout: player (left), enemy (right), scrollable type rows
    float colW=(SCREEN_W-20)/2.0f;
    float listTop=60.0f, rowH=44.0f;
    int nc=unitTypeCount();

    // Headers
    DrawText("YOUR ARMY (Blue)",(int)(14),(int)(listTop+4),15,{80,140,255,255});
    DrawText("ENEMY ARMY (Red)",(int)(colW+14),(int)(listTop+4),15,{255,100,100,255});
    DrawLine(0,(int)(listTop+26),SCREEN_W,(int)(listTop+26),{50,80,50,120});
    listTop+=30;

    // Count player / enemy totals
    int pTotal=0,eTotal=0;
    for(int v:cfg.playerCounts) pTotal+=v;
    for(int v:cfg.enemyCounts) eTotal+=v;

    for(int i=0;i<nc;i++){
        if(i>=20) break; // max display
        const UnitTypeDef& td=g_unitTypes[i];
        float ry=listTop+i*rowH;
        if(ry+rowH > SCREEN_H-80) break;

        // Player side
        {
            Color rowbg=(i%2==0)?Color{10,18,30,200}:Color{8,14,24,200};
            DrawRectangle(0,(int)ry,(int)(colW-2),(int)rowH,rowbg);
            Color typeCol={td.r,td.g,td.b,255};
            DrawRectangle(8,(int)(ry+5),14,14,typeCol);
            DrawText(td.name,28,(int)(ry+8),13,WHITE);
            DrawText(TextFormat("HP:%d",(int)(td.entities*td.hpPerEntity)),(int)(colW-230),(int)(ry+8),12,{120,200,120,255});
            DrawText(TextFormat("G:%d",td.cost),(int)(colW-160),(int)(ry+8),12,{200,200,80,255});
            // stepper
            float bx2=colW-90;
            bool mhv=CheckCollisionPointRec(mouse,{bx2,ry+8,22,22});
            bool phv=CheckCollisionPointRec(mouse,{bx2+50,ry+8,22,22});
            DrawRectangle((int)bx2,(int)(ry+8),22,22,mhv?Color{80,60,30,255}:Color{40,30,15,255});
            DrawRectangleLinesEx({bx2,ry+8,22,22},1,{100,80,40,255});
            DrawText("-",(int)(bx2+7),(int)(ry+10),16,WHITE);
            if(mhv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)&&cfg.playerCounts[i]>0)
                cfg.playerCounts[i]--;

            DrawText(TextFormat("%d",cfg.playerCounts[i]),(int)(bx2+26),(int)(ry+10),15,WHITE);

            DrawRectangle((int)(bx2+50),(int)(ry+8),22,22,phv?Color{30,80,30,255}:Color{15,40,15,255});
            DrawRectangleLinesEx({bx2+50,ry+8,22,22},1,{40,100,40,255});
            DrawText("+",(int)(bx2+56),(int)(ry+10),16,GREEN);
            if(phv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)&&pTotal<20)
                cfg.playerCounts[i]++;
        }
        // Enemy side
        {
            float ex2=colW+2;
            Color rowbg=(i%2==0)?Color{28,8,8,200}:Color{22,6,6,200};
            DrawRectangle((int)ex2,(int)ry,(int)(colW-2),(int)rowH,rowbg);
            unsigned char er=clampU8((int)(td.r*0.4f+180));
            unsigned char eg=clampU8((int)(td.g*0.25f));
            unsigned char eb2=clampU8((int)(td.b*0.25f));
            Color typeCol={er,eg,eb2,255};
            DrawRectangle((int)(ex2+8),(int)(ry+5),14,14,typeCol);
            DrawText(td.name,(int)(ex2+28),(int)(ry+8),13,WHITE);
            DrawText(TextFormat("HP:%d",(int)(td.entities*td.hpPerEntity)),(int)(ex2+colW-230),(int)(ry+8),12,{200,120,120,255});

            float bxe=ex2+colW-90;
            bool mhv=CheckCollisionPointRec(mouse,{bxe,ry+8,22,22});
            bool phv=CheckCollisionPointRec(mouse,{bxe+50,ry+8,22,22});
            DrawRectangle((int)bxe,(int)(ry+8),22,22,mhv?Color{80,30,30,255}:Color{40,15,15,255});
            DrawRectangleLinesEx({bxe,ry+8,22,22},1,{100,40,40,255});
            DrawText("-",(int)(bxe+7),(int)(ry+10),16,WHITE);
            if(mhv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)&&cfg.enemyCounts[i]>0)
                cfg.enemyCounts[i]--;

            DrawText(TextFormat("%d",cfg.enemyCounts[i]),(int)(bxe+26),(int)(ry+10),15,WHITE);

            DrawRectangle((int)(bxe+50),(int)(ry+8),22,22,phv?Color{80,30,30,255}:Color{40,15,15,255});
            DrawRectangleLinesEx({bxe+50,ry+8,22,22},1,{120,40,40,255});
            DrawText("+",(int)(bxe+56),(int)(ry+10),16,RED);
            if(phv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)&&eTotal<20)
                cfg.enemyCounts[i]++;
        }
    }

    // Divider between columns
    DrawLine((int)(colW),(int)listTop,(int)colW,SCREEN_H-76,{50,80,50,160});

    // Bottom bar
    int barY=SCREEN_H-74;
    DrawRectangle(0,barY,SCREEN_W,74,{0,0,0,220});
    DrawLine(0,barY,SCREEN_W,barY,{50,80,50,140});

    // Difficulty selector
    DrawText("Difficulty:",(int)14,(int)(barY+10),13,LIGHTGRAY);
    const char* diffs[3]={"EASY","NORMAL","HARD"};
    Color dcols[3]={GREEN,YELLOW,RED};
    for(int d=0;d<3;d++){
        bool sel=(cfg.difficulty==(Difficulty)d);
        Rectangle dr={14.0f+d*100.0f,(float)(barY+26),90,24};
        bool hv=CheckCollisionPointRec(mouse,dr);
        DrawRectangleRec(dr,sel?Color{30,80,30,255}:hv?Color{22,50,22,255}:Color{14,28,14,220});
        DrawRectangleLinesEx(dr,1,sel?dcols[d]:Color{40,70,40,255});
        int tw2=MeasureText(diffs[d],13);
        DrawText(diffs[d],(int)(dr.x+45-tw2/2),(int)(dr.y+5),13,sel?dcols[d]:LIGHTGRAY);
        if(hv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) cfg.difficulty=(Difficulty)d;
    }

    // Totals
    DrawText(TextFormat("Your: %d units",pTotal),(int)330,(int)(barY+10),13,{80,140,255,255});
    DrawText(TextFormat("Enemy: %d units",eTotal),(int)330,(int)(barY+28),13,{255,100,100,255});

    bool canStart=(pTotal>0&&eTotal>0);
    Color cn=canStart?Color{30,70,30,255}:Color{35,35,35,255};
    Color ch=canStart?Color{60,140,60,255}:Color{35,35,35,255};
    if(drawButton({(float)(SCREEN_W-420),(float)(barY+10),280,50},"START BATTLE",mouse,cn,ch)&&canStart)
        return STATE_PLAYING;
    if(!canStart)
        DrawText("Add units to both sides",(int)(SCREEN_W-420),(int)(barY+62),12,{180,90,40,255});
    if(drawButton({(float)(SCREEN_W-128),(float)(barY+10),116,50},"BACK",mouse,{40,25,25,255},{80,40,40,255}))
        return STATE_MENU;

    return STATE_CUSTOM_BATTLE;
}

// ═══════════════════════════════════════════════════════════════
//  UNIT EDITOR
// ═══════════════════════════════════════════════════════════════
static int    g_editTypeIdx=0;
static bool   g_dropdownOpen=false;
static bool   g_previewDirty=true;
static Texture2D g_previewTex={0};
static float  g_previewAngle=0;

// For creating new types
static bool   g_creatingNew=false;
static UnitTypeDef g_newDraft{};
static bool   g_nameActive=false;
static Texture2D g_draftTex={0};
static float  g_draftAngle=0;

void rebuildPreviewTex(){
    if(g_previewTex.id>0) UnloadTexture(g_previewTex);
    g_previewTex={0};
    if(g_editTypeIdx>=0&&g_editTypeIdx<unitTypeCount()){
        const UnitTypeDef& td=g_unitTypes[g_editTypeIdx];
        Image img;
        switch(td.spriteBase){
            case SPR_CAVALRY: img=makeCavalryImg(td.r,td.g,td.b); break;
            case SPR_RANGED:  img=makeRangedImg(td.r,td.g,td.b,td.missileReload>2.5f); break;
            default:          img=makeInfantryImg(td.r,td.g,td.b,td.weaponHint); break;
        }
        g_previewTex=imgToTex(img);
    }
}

void rebuildDraftTex(const UnitTypeDef& td){
    if(g_draftTex.id>0) UnloadTexture(g_draftTex);
    g_draftTex={0};
    Image img;
    switch(td.spriteBase){
        case SPR_CAVALRY: img=makeCavalryImg(td.r,td.g,td.b); break;
        case SPR_RANGED:  img=makeRangedImg(td.r,td.g,td.b,td.missileReload>2.5f); break;
        default:          img=makeInfantryImg(td.r,td.g,td.b,td.weaponHint); break;
    }
    g_draftTex=imgToTex(img);
}

// Draw a simple bar chart of stats (normalized)
void drawStatBars(float x,float y,float w,float bh,const UnitTypeDef& td){
    struct Stat { const char* name; float val; float maxv; Color col; };
    float totalHP=(float)(td.entities*td.hpPerEntity);
    Stat stats[]={
        {"HP",     totalHP, 18000.0f, {60,220,60,255}},
        {"Armor",  (float)td.armor, 40.0f, {160,200,220,255}},
        {"Speed",  (float)td.speed, 200.0f, {60,180,220,255}},
        {"M.Atk",  (float)td.meleeAttack, 80.0f, {220,160,40,255}},
        {"M.Def",  (float)td.meleeDefense, 60.0f, {180,120,40,255}},
        {"MelDmg", (float)(td.meleeBaseDmg+td.meleeAPDmg), 140.0f, {220,80,40,255}},
        {"Range",  (float)td.range, 500.0f, {200,200,80,255}},
        {"MisDmg", (float)(td.missileBaseDmg+td.missileAPDmg), 140.0f, {200,100,200,255}},
    };
    int n=8;
    float rh=bh/n;
    for(int i=0;i<n;i++){
        float fy=y+i*rh;
        DrawText(stats[i].name,(int)x,(int)(fy+1),11,LIGHTGRAY);
        float barX=x+60, barW=w-70;
        DrawRectangle((int)barX,(int)(fy+1),(int)barW,(int)(rh-3),{20,20,20,255});
        float ratio=std::min(1.0f,stats[i].val/stats[i].maxv);
        DrawRectangle((int)barX,(int)(fy+1),(int)(barW*ratio),(int)(rh-3),stats[i].col);
        DrawText(TextFormat("%.0f",stats[i].val),(int)(barX+barW+4),(int)(fy+1),11,WHITE);
    }
}

GameState updateDrawUnitEditor(Vector2 mouse,float dt){
    if(g_editTypeIdx>=unitTypeCount()) g_editTypeIdx=0;

    g_previewAngle+=40.0f*dt;
    g_draftAngle+=40.0f*dt;

    ClearBackground({8,10,18,255});
    for(int gx=0;gx<SCREEN_W;gx+=40) DrawLine(gx,0,gx,SCREEN_H,{18,20,40,70});
    for(int gy=0;gy<SCREEN_H;gy+=40) DrawLine(0,gy,SCREEN_W,gy,{18,20,40,70});

    DrawRectangle(0,0,SCREEN_W,54,{0,0,0,220});
    DrawText("UNIT EDITOR",16,10,28,GREEN);
    DrawText("Edit unit stats — changes apply immediately to future battles",16,40,12,{100,160,100,255});

    int headerH=54, panelY=headerH+6, panelH=SCREEN_H-panelY-54;
    int leftW=490, rightX=leftW+20, rightW=SCREEN_W-rightX-10;

    DrawRectangle(10,panelY,leftW,panelH,{10,16,12,210});
    DrawRectangleLinesEx({10,(float)panelY,(float)leftW,(float)panelH},1,{50,90,50,200});
    DrawRectangle(rightX,panelY,rightW,panelH,{10,12,28,210});
    DrawRectangleLinesEx({(float)rightX,(float)panelY,(float)rightW,(float)panelH},1,{50,50,110,200});

    float lx=20, ly=(float)(panelY+12);
    float sliderW=(float)(leftW-80);

    // ── Dropdown ──────────────────────────────────────────────
    DrawText("Unit type:",(int)lx,(int)ly,13,LIGHTGRAY); ly+=18;
    if(unitTypeCount()>0){
        const UnitTypeDef& cur=g_unitTypes[g_editTypeIdx];
        Rectangle dropBtn={lx,ly,(float)(leftW-20),26};
        bool dHv=ptInRect(mouse,dropBtn);
        DrawRectangleRec(dropBtn,dHv?Color{28,68,48,255}:Color{16,38,28,255});
        DrawRectangleLinesEx(dropBtn,1,g_dropdownOpen?GREEN:Color{50,90,60,255});
        Color patch={cur.r,cur.g,cur.b,255};
        DrawRectangle((int)lx+4,(int)ly+5,14,14,patch);
        DrawText(cur.name,(int)(lx+24),(int)(ly+5),14,WHITE);
        int trx=(int)(lx+leftW-30), tty=(int)(ly+13);
        DrawTriangle({(float)trx,(float)(tty-5)},{(float)(trx-6),(float)(tty+5)},
                     {(float)(trx+6),(float)(tty+5)},g_dropdownOpen?GREEN:LIGHTGRAY);
        if(dHv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) g_dropdownOpen=!g_dropdownOpen;

        if(g_dropdownOpen){
            float oy=ly+28;
            for(int i=0;i<unitTypeCount();i++){
                const UnitTypeDef& td=g_unitTypes[i];
                bool sel=(i==g_editTypeIdx);
                Rectangle opt={lx,oy,(float)(leftW-20),24};
                bool ohv=ptInRect(mouse,opt);
                DrawRectangleRec(opt,sel?Color{28,78,40,255}:ohv?Color{20,48,30,255}:Color{12,28,20,240});
                DrawRectangleLinesEx(opt,1,sel?GREEN:Color{40,68,45,255});
                Color op={td.r,td.g,td.b,255};
                DrawRectangle((int)lx+4,(int)(oy+4),12,12,op);
                DrawText(td.name,(int)(lx+22),(int)(oy+4),13,sel?GREEN:WHITE);
                if(ohv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                    g_editTypeIdx=i; g_dropdownOpen=false; g_previewDirty=true;
                }
                oy+=25;
            }
            if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                float listH=25.0f*unitTypeCount();
                if(!ptInRect(mouse,{lx,ly,(float)(leftW-20),28+listH}))
                    g_dropdownOpen=false;
            }
            ly+=(float)(25*unitTypeCount())+32;
        } else {
            ly+=32;
        }
    }

    if(g_editTypeIdx<unitTypeCount()){
        UnitTypeDef& td=g_unitTypes[g_editTypeIdx];
        bool changed=false;

        DrawLine((int)lx,(int)ly,(int)(lx+leftW-20),(int)ly,{50,90,50,100}); ly+=10;

        // Stats sliders — inline to avoid macro/Color brace-comma issues
        DrawText("STATS",(int)lx,(int)ly,12,{80,180,80,255}); ly+=16;
        auto SI=[&](int& field,int mn,int mx,const char* lbl,Color col){
            int nv=drawIntSlider({lx,ly,sliderW,16},field,mn,mx,lbl,mouse,col);
            if(nv!=field){field=nv;changed=true;} ly+=34;
        };
        auto SF=[&](float& field,float mn,float mx,const char* lbl,const char* fmt,Color col){
            float nv=drawFloatSlider({lx,ly,sliderW,16},field,mn,mx,lbl,fmt,mouse,col);
            if(nv!=field){field=nv;changed=true;} ly+=34;
        };
        SI(td.entities,       1,  120,"Entities",             {80,180,80,200});
        SI(td.hpPerEntity,   10,  300,"HP per Entity",         {60,220,60,200});
        SI(td.armor,          0,   40,"Armor",                 {160,200,220,200});
        SI(td.speed,         30,  200,"Speed",                 {60,180,220,200});
        SI(td.meleeAttack,    1,   80,"Melee Attack",          {220,180,40,200});
        SI(td.meleeDefense,   0,   60,"Melee Defense",         {180,140,40,200});
        SI(td.meleeBaseDmg,   1,   80,"Melee Base Dmg",        {220,100,40,200});
        SI(td.meleeAPDmg,     0,   60,"Melee AP Dmg",          {220,60,40,200});
        SF(td.meleeInterval,0.5f,4.0f,"Melee Interval (s)","%.1fs",{180,80,80,200});
        SI(td.range,          0,  500,"Range (0=melee only)",  {200,200,80,200});
        SI(td.missileBaseDmg, 0,   80,"Missile Base Dmg",      {180,80,200,200});
        SI(td.missileAPDmg,   0,   60,"Missile AP Dmg",        {160,60,200,200});
        SF(td.missileReload,0.5f,8.0f,"Missile Reload (s)","%.1fs",{160,80,180,200});
        SI(td.cost,          10,  500,"Cost (gold, display)",  {200,200,60,200});

        // Color
        DrawLine((int)lx,(int)ly,(int)(lx+leftW-20),(int)ly,{50,90,50,100}); ly+=8;
        DrawText("COLOR",(int)lx,(int)ly,12,{80,180,80,255}); ly+=16;
        int nr=drawIntSlider({lx+16,ly,sliderW,14},(int)td.r,0,255,"R",mouse,{220,60,60,200});
        DrawRectangle((int)lx,(int)ly,12,14,{(unsigned char)nr,0,0,255}); ly+=28;
        int ng=drawIntSlider({lx+16,ly,sliderW,14},(int)td.g,0,255,"G",mouse,{60,220,60,200});
        DrawRectangle((int)lx,(int)ly,12,14,{0,(unsigned char)ng,0,255}); ly+=28;
        int nb=drawIntSlider({lx+16,ly,sliderW,14},(int)td.b,0,255,"B",mouse,{60,60,220,200});
        DrawRectangle((int)lx,(int)ly,12,14,{0,0,(unsigned char)nb,255}); ly+=32;
        if((unsigned char)nr!=td.r||(unsigned char)ng!=td.g||(unsigned char)nb!=td.b){
            td.r=(unsigned char)nr; td.g=(unsigned char)ng; td.b=(unsigned char)nb; changed=true;
        }
        DrawRectangle((int)lx,(int)ly,48,18,{td.r,td.g,td.b,255});
        DrawRectangleLinesEx({(float)lx,(float)ly,48,18},1,WHITE);
        DrawText(TextFormat("RGB(%d,%d,%d)",td.r,td.g,td.b),(int)(lx+54),(int)(ly+3),12,LIGHTGRAY);
        ly+=28;

        // Sprite base
        DrawText("Sprite type:",(int)lx,(int)ly,12,LIGHTGRAY); ly+=16;
        for(int s=0;s<SPR_BASE_COUNT;s++){
            bool sel=(td.spriteBase==(SpriteBase)s);
            Rectangle sb={lx+s*130.0f,ly,120,22};
            bool shv=ptInRect(mouse,sb);
            DrawRectangleRec(sb,sel?Color{20,60,100,255}:shv?Color{16,40,65,255}:Color{10,24,44,255});
            DrawRectangleLinesEx(sb,1,sel?Color{80,180,255,255}:Color{40,80,120,255});
            int stw=MeasureText(spriteBaseNames[s],12);
            DrawText(spriteBaseNames[s],(int)(sb.x+60-stw/2),(int)(sb.y+4),12,sel?WHITE:LIGHTGRAY);
            if(shv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){td.spriteBase=(SpriteBase)s;changed=true;}
        }

        if(changed){ rebuildTexture(g_editTypeIdx); g_previewDirty=true; }
        if(g_previewDirty){ rebuildPreviewTex(); g_previewDirty=false; }

        // ── RIGHT PANEL ──────────────────────────────────────
        float rx=(float)rightX, ry=(float)panelY, rw=(float)rightW, rh2=(float)panelH;
        DrawText(TextFormat("PREVIEW: %s",td.name),(int)(rx+12),(int)(ry+12),15,SKYBLUE);
        DrawLine((int)rx,(int)(ry+34),(int)(rx+rw),(int)(ry+34),{50,50,110,100});

        // Sprite preview center
        Vector2 center={rx+rw/2, ry+rh2*0.22f};
        if(g_previewTex.id>0){
            drawSprite(g_previewTex,center,g_previewAngle,10.0f,{td.r,td.g,td.b,255});
            DrawCircleLines((int)center.x,(int)center.y,68,{100,100,200,80});
        }

        // Derived stats
        float sy=ry+rh2*0.42f;
        DrawText("DERIVED STATS",(int)(rx+12),(int)sy,13,{100,180,255,255}); sy+=18;
        DrawText(TextFormat("Total HP: %d",(int)(td.entities*td.hpPerEntity)),(int)(rx+12),(int)sy,13,{60,220,60,255}); sy+=17;
        DrawText(TextFormat("Total Melee Dmg: %d",td.meleeBaseDmg+td.meleeAPDmg),(int)(rx+12),(int)sy,13,{220,160,40,255}); sy+=17;
        DrawText(TextFormat("Total Missile Dmg: %d",td.missileBaseDmg+td.missileAPDmg),(int)(rx+12),(int)sy,13,{200,100,200,255}); sy+=17;
        DrawText(TextFormat("Gold cost: %d",td.cost),(int)(rx+12),(int)sy,13,{200,200,80,255}); sy+=17;
        DrawText(td.isBuiltin?"Built-in unit":"Custom unit",(int)(rx+12),(int)sy,12,{120,120,80,255}); sy+=20;

        // Stat bar chart
        DrawLine((int)rx,(int)sy,(int)(rx+rw),(int)sy,{50,50,110,100}); sy+=8;
        DrawText("STAT BARS (normalized)",(int)(rx+12),(int)sy,12,{100,180,255,200}); sy+=16;
        drawStatBars(rx+12,sy,rw-24,(float)(panelY+panelH-sy-10),td);
    }

    // Bottom buttons
    int btnY=SCREEN_H-48;
    if(!g_creatingNew){
        if(drawSmallButton({10,(float)btnY,180,36},"+ CREATE NEW UNIT",mouse,{20,48,80,255},{38,88,148,255})){
            memset(&g_newDraft,0,sizeof(g_newDraft));
            strcpy(g_newDraft.name,"NewUnit");
            g_newDraft.entities=40; g_newDraft.hpPerEntity=80; g_newDraft.armor=5;
            g_newDraft.speed=100; g_newDraft.meleeAttack=20; g_newDraft.meleeDefense=14;
            g_newDraft.meleeBaseDmg=15; g_newDraft.meleeAPDmg=5; g_newDraft.meleeInterval=1.6f;
            g_newDraft.range=0; g_newDraft.cost=100;
            g_newDraft.r=160; g_newDraft.g=80; g_newDraft.b=200;
            g_newDraft.spriteBase=SPR_INFANTRY; g_newDraft.isBuiltin=false; g_newDraft.weaponHint=0;
            g_creatingNew=true; g_nameActive=true;
            rebuildDraftTex(g_newDraft);
        }
        if(g_editTypeIdx<unitTypeCount()&&!g_unitTypes[g_editTypeIdx].isBuiltin){
            if(drawSmallButton({200,(float)btnY,120,36},"DELETE UNIT",mouse,{80,20,20,255},{140,40,40,255})){
                g_unitTypes.erase(g_unitTypes.begin()+g_editTypeIdx);
                g_playerTextures.erase(g_playerTextures.begin()+g_editTypeIdx);
                g_enemyTextures.erase(g_enemyTextures.begin()+g_editTypeIdx);
                g_editTypeIdx=std::max(0,g_editTypeIdx-1);
                g_previewDirty=true;
            }
        }
        if(drawSmallButton({330,(float)btnY,100,36},"BACK",mouse,{40,25,25,255},{80,40,40,255}))
            return STATE_MENU;
    }

    // ── CREATE NEW UNIT DIALOG ──────────────────────────────────
    if(g_creatingNew){
        DrawRectangle(0,0,SCREEN_W,SCREEN_H,{0,0,0,190});
        float dw=720, dh=560, dx=(SCREEN_W-dw)/2, dy=(SCREEN_H-dh)/2;
        DrawRectangle((int)dx,(int)dy,(int)dw,(int)dh,{10,16,28,248});
        DrawRectangleLinesEx({dx,dy,dw,dh},2,{80,140,220,255});
        DrawText("CREATE NEW UNIT TYPE",(int)(dx+18),(int)(dy+14),18,{80,160,255,255});
        DrawLine((int)dx,(int)(dy+40),(int)(dx+dw),(int)(dy+40),{80,140,220,80});

        float cx=dx+20, cy=dy+54;
        // Name field
        DrawText("Name:",(int)cx,(int)cy,13,LIGHTGRAY); cy+=18;
        Rectangle nameBox={cx,cy,260,26};
        bool nHv=ptInRect(mouse,nameBox);
        DrawRectangleRec(nameBox,g_nameActive?Color{18,40,60,255}:Color{12,24,40,255});
        DrawRectangleLinesEx(nameBox,1,g_nameActive?Color{100,180,255,255}:Color{50,90,130,255});
        DrawText(g_newDraft.name,(int)(cx+5),(int)(cy+5),13,WHITE);
        if(g_nameActive) DrawText("|",(int)(cx+6+MeasureText(g_newDraft.name,13)),(int)(cy+4),13,WHITE);
        if(nHv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) g_nameActive=true;
        else if(!nHv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) g_nameActive=false;
        if(g_nameActive){
            int key=GetCharPressed();
            while(key>0){
                int len=strlen(g_newDraft.name);
                if(key>=32&&len<30){ g_newDraft.name[len]=(char)key; g_newDraft.name[len+1]='\0'; }
                key=GetCharPressed();
            }
            if(IsKeyPressed(KEY_BACKSPACE)){
                int len=strlen(g_newDraft.name);
                if(len>0) g_newDraft.name[len-1]='\0';
            }
        }
        cy+=34;

        // Sprite base
        DrawText("Sprite base:",(int)cx,(int)cy,13,LIGHTGRAY); cy+=18;
        for(int s=0;s<SPR_BASE_COUNT;s++){
            bool sel=(g_newDraft.spriteBase==(SpriteBase)s);
            Rectangle sb={cx+s*140.0f,cy,130,24};
            bool shv=ptInRect(mouse,sb);
            DrawRectangleRec(sb,sel?Color{20,60,100,255}:shv?Color{16,40,65,255}:Color{10,24,44,255});
            DrawRectangleLinesEx(sb,1,sel?Color{80,180,255,255}:Color{40,80,120,255});
            int stw=MeasureText(spriteBaseNames[s],13);
            DrawText(spriteBaseNames[s],(int)(sb.x+65-stw/2),(int)(sb.y+5),13,sel?WHITE:LIGHTGRAY);
            if(shv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                g_newDraft.spriteBase=(SpriteBase)s; rebuildDraftTex(g_newDraft);
            }
        }
        cy+=34;

        float sw3=250;
        // Use lambdas to avoid macro/Color brace-comma issues
        auto DSI=[&](int& field,int mn,int mx,const char* lbl,Color col){
            int nv2=drawIntSlider({cx,cy,sw3,15},field,mn,mx,lbl,mouse,col);
            if(nv2!=field){field=nv2;rebuildDraftTex(g_newDraft);} cy+=32;
        };
        DSI(g_newDraft.entities,      1, 120,"Entities",         {80,180,80,200});
        DSI(g_newDraft.hpPerEntity,  10, 300,"HP/Entity",        {60,220,60,200});
        DSI(g_newDraft.armor,         0,  40,"Armor",            {160,200,220,200});
        DSI(g_newDraft.speed,        30, 200,"Speed",            {60,180,220,200});
        DSI(g_newDraft.meleeBaseDmg,  1,  80,"Melee Base Dmg",   {220,100,40,200});
        DSI(g_newDraft.range,         0, 500,"Range (0=melee)",  {200,200,80,200});
        DSI(g_newDraft.cost,         10, 500,"Gold cost",        {200,200,60,200});

        // Color sliders
        int nr2=drawIntSlider({cx+16,cy,sw3,14},(int)g_newDraft.r,0,255,"R",mouse,{220,60,60,200});
        DrawRectangle((int)cx,(int)cy,12,14,{(unsigned char)nr2,0,0,255}); cy+=26;
        int ng2=drawIntSlider({cx+16,cy,sw3,14},(int)g_newDraft.g,0,255,"G",mouse,{60,220,60,200});
        DrawRectangle((int)cx,(int)cy,12,14,{0,(unsigned char)ng2,0,255}); cy+=26;
        int nb2=drawIntSlider({cx+16,cy,sw3,14},(int)g_newDraft.b,0,255,"B",mouse,{60,60,220,200});
        DrawRectangle((int)cx,(int)cy,12,14,{0,0,(unsigned char)nb2,255}); cy+=26;
        if((unsigned char)nr2!=g_newDraft.r||(unsigned char)ng2!=g_newDraft.g||(unsigned char)nb2!=g_newDraft.b){
            g_newDraft.r=(unsigned char)nr2; g_newDraft.g=(unsigned char)ng2; g_newDraft.b=(unsigned char)nb2;
            rebuildDraftTex(g_newDraft);
        }

        // Preview on right side of dialog
        if(g_draftTex.id>0){
            Vector2 dc={dx+dw-140, dy+200};
            drawSprite(g_draftTex,dc,g_draftAngle,9.0f,{g_newDraft.r,g_newDraft.g,g_newDraft.b,255});
            DrawCircleLines((int)dc.x,(int)dc.y,56,{80,140,220,80});
            DrawText(TextFormat("Ent:%d  HP:%d",g_newDraft.entities,(int)(g_newDraft.entities*g_newDraft.hpPerEntity)),
                     (int)(dc.x-60),(int)(dc.y+70),12,LIGHTGRAY);
        }

        bool nameOk=(strlen(g_newDraft.name)>0);
        Color cCN=nameOk?Color{20,70,20,255}:Color{30,30,30,255};
        Color cCH=nameOk?Color{40,130,40,255}:Color{30,30,30,255};
        if(drawButton({dx+18,dy+dh-52,200,40},"CREATE UNIT",mouse,cCN,cCH)&&nameOk){
            g_newDraft.isBuiltin=false;
            g_unitTypes.push_back(g_newDraft);
            int newIdx=(int)g_unitTypes.size()-1;
            rebuildTexture(newIdx);
            g_editTypeIdx=newIdx;
            g_creatingNew=false; g_previewDirty=true;
            if(g_draftTex.id>0){UnloadTexture(g_draftTex);g_draftTex={0};}
            rebuildPreviewTex();
        }
        if(drawButton({dx+236,dy+dh-52,160,40},"CANCEL",mouse,{60,25,25,255},{110,45,45,255})){
            g_creatingNew=false;
            if(g_draftTex.id>0){UnloadTexture(g_draftTex);g_draftTex={0};}
        }
    }

    return STATE_UNIT_EDITOR;
}

// ═══════════════════════════════════════════════════════════════
//  INIT GAME
// ═══════════════════════════════════════════════════════════════
void initGame(std::vector<BUnit>& playerUnits,
              std::vector<BUnit>& enemyUnits,
              std::vector<Projectile>& projs,
              std::vector<DeadMarker>& dead,
              const BattleConfig& cfg){
    playerUnits.clear(); enemyUnits.clear(); projs.clear(); dead.clear();

    // Spawn player units left side
    float startX=60, startY=100;
    int row=0,col=0;
    for(int t=0;t<unitTypeCount();t++){
        if(t>=(int)cfg.playerCounts.size()) break;
        int cnt=cfg.playerCounts[t];
        if(cnt==0) continue;
        const UnitTypeDef& td=g_unitTypes[t];
        for(int i=0;i<cnt;i++){
            BUnit u{};
            u.pos={startX+col*56.0f, startY+row*60.0f};
            u.target=u.pos; u.orderTarget=-1;
            u.hp=u.maxHp=(float)(td.entities*td.hpPerEntity);
            u.alive=true; u.isPlayer=true;
            u.typeIdx=t; u.angle=0; u.angleTarget=0;
            u.chargeReady=(td.spriteBase==SPR_CAVALRY);
            u.action=ACT_IDLE;
            playerUnits.push_back(u);
            col++; if(col>=3){col=0;row++;}
        }
    }

    // Spawn enemy units right side
    float ex0=SCREEN_W-60; row=0; col=0;
    for(int t=0;t<unitTypeCount();t++){
        if(t>=(int)cfg.enemyCounts.size()) break;
        int cnt=cfg.enemyCounts[t];
        if(cnt==0) continue;
        const UnitTypeDef& td=g_unitTypes[t];
        for(int i=0;i<cnt;i++){
            BUnit u{};
            u.pos={ex0-col*56.0f, startY+row*60.0f};
            u.target=u.pos; u.orderTarget=-1;
            u.hp=u.maxHp=(float)(td.entities*td.hpPerEntity);
            u.alive=true; u.isPlayer=false;
            u.typeIdx=t; u.angle=180; u.angleTarget=180;
            u.chargeReady=(td.spriteBase==SPR_CAVALRY);
            u.action=ACT_IDLE;
            enemyUnits.push_back(u);
            col++; if(col>=3){col=0;row++;}
        }
    }
}

// ═══════════════════════════════════════════════════════════════
//  COMBAT HELPERS
// ═══════════════════════════════════════════════════════════════
static inline float meleeRadius(const UnitTypeDef& td){
    return td.spriteBase==SPR_CAVALRY?20.0f:14.0f;
}

float calcMeleeDmg(const UnitTypeDef& attDef,const UnitTypeDef& defDef,
                   const BUnit& att,bool chargeBonus=false){
    // hit chance
    float raw=35.0f+(float)attDef.meleeAttack-(float)defDef.meleeDefense;
    float hitPct=std::max(10.0f,std::min(90.0f,raw));
    if((float)(rand()%100)>=hitPct) return 0.0f; // miss
    // armor
    float armRed=frand()*0.5f*(float)defDef.armor+(float)defDef.armor*0.5f;
    armRed=std::min(armRed,(float)defDef.armor);
    float effBase=(float)attDef.meleeBaseDmg*(1.0f-armRed/100.0f);
    float total=effBase+(float)attDef.meleeAPDmg;
    if(chargeBonus) total*=2.0f;
    return std::max(0.5f,total);
}

float calcMissileDmg(const UnitTypeDef& attDef,const UnitTypeDef& defDef){
    float armRed=frand()*0.5f*(float)defDef.armor+(float)defDef.armor*0.5f;
    armRed=std::min(armRed,(float)defDef.armor);
    float effBase=(float)attDef.missileBaseDmg*(1.0f-armRed/100.0f);
    return std::max(0.0f,effBase+(float)attDef.missileAPDmg);
}

// Check line-of-sight clear of friendlies (12px corridor)
bool losClean(Vector2 from,Vector2 to,const std::vector<BUnit>& friendlies,int selfIdx){
    Vector2 dir=vnorm({to.x-from.x,to.y-from.y});
    float dist=vdist(from,to);
    for(int i=0;i<(int)friendlies.size();i++){
        if(i==selfIdx||!friendlies[i].alive) continue;
        // project onto line
        Vector2 rel={friendlies[i].pos.x-from.x,friendlies[i].pos.y-from.y};
        float proj=rel.x*dir.x+rel.y*dir.y;
        if(proj<0||proj>dist) continue;
        float perp=fabsf(rel.x*(-dir.y)+rel.y*dir.x);
        if(perp<12.0f) return false;
    }
    return true;
}

// Soft push separation between same team
void separateUnits(std::vector<BUnit>& units){
    for(int i=0;i<(int)units.size();i++){
        if(!units[i].alive) continue;
        for(int j=i+1;j<(int)units.size();j++){
            if(!units[j].alive) continue;
            float d=vdist(units[i].pos,units[j].pos);
            float minD=24.0f;
            if(d<minD&&d>0.001f){
                float push=(minD-d)*0.3f;
                Vector2 dir=vnorm({units[j].pos.x-units[i].pos.x,units[j].pos.y-units[i].pos.y});
                units[i].pos={units[i].pos.x-dir.x*push,units[i].pos.y-dir.y*push};
                units[j].pos={units[j].pos.x+dir.x*push,units[j].pos.y+dir.y*push};
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════
//  MAIN GAME UPDATE/DRAW
// ═══════════════════════════════════════════════════════════════
static bool g_paused=false;
static bool g_gameOver=false;
static bool g_victory=false;

GameState updateDrawGame(std::vector<BUnit>& playerUnits,
                         std::vector<BUnit>& enemyUnits,
                         std::vector<Projectile>& projs,
                         std::vector<DeadMarker>& dead,
                         const BattleConfig& cfg,
                         Vector2 mouse,float dt){

    static Vector2 selStart={};
    static bool dragging=false;
    static Rectangle selRect={};

    // ── PAUSE OVERLAY ────────────────────────────────────────
    if(g_paused){
        ClearBackground({10,14,10,255});
        DrawRectangle(0,0,SCREEN_W,SCREEN_H,{0,0,0,180});
        const char* pm="PAUSED";
        int ptw=MeasureText(pm,56);
        DrawText(pm,SCREEN_W/2-ptw/2,SCREEN_H/2-80,56,YELLOW);
        if(drawButton({(float)(SCREEN_W/2-120),(float)(SCREEN_H/2+10),240,50},"RESUME",mouse,{30,70,30,255},{60,130,60,255}))
            g_paused=false;
        if(drawButton({(float)(SCREEN_W/2-120),(float)(SCREEN_H/2+70),240,50},"RETURN TO MENU",mouse,{60,30,30,255},{100,50,50,255})){
            g_paused=false; return STATE_MENU;
        }
        return STATE_PLAYING;
    }

    if(IsKeyPressed(KEY_ESCAPE)) g_paused=true;

    // ── SELECTION / MOVEMENT CONTROLS ────────────────────────
    bool overHud=mouse.y>BATTLEFIELD_H;
    bool overMenu=ptInRect(mouse,{(float)(SCREEN_W-134),4,126,32});

    if(!overHud&&!overMenu){
        if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
            selStart=mouse; dragging=true;
            for(auto& u:playerUnits) u.selected=false;
        }
        if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)&&dragging){
            selRect.x=fminf(mouse.x,selStart.x);
            selRect.y=fminf(mouse.y,selStart.y);
            selRect.width=fabsf(mouse.x-selStart.x);
            selRect.height=fabsf(mouse.y-selStart.y);
        }
        if(IsMouseButtonReleased(MOUSE_LEFT_BUTTON)){
            dragging=false;
            if(selRect.width<5&&selRect.height<5){
                // single click
                float best=9999; int idx=-1;
                for(int i=0;i<(int)playerUnits.size();i++){
                    if(!playerUnits[i].alive) continue;
                    float d=vdist(mouse,playerUnits[i].pos);
                    if(d<best&&d<24){best=d;idx=i;}
                }
                if(idx>=0) playerUnits[idx].selected=true;
            } else {
                for(auto& u:playerUnits)
                    if(u.alive&&ptInRect(u.pos,selRect)) u.selected=true;
            }
            selRect={};
        }
        if(IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)){
            // Check if clicking on an enemy for explicit attack order
            int eIdx=-1; float bestD=9999;
            for(int i=0;i<(int)enemyUnits.size();i++){
                if(!enemyUnits[i].alive) continue;
                float d=vdist(mouse,enemyUnits[i].pos);
                if(d<24&&d<bestD){bestD=d;eIdx=i;}
            }
            int offX=0;
            for(auto& u:playerUnits){
                if(!u.selected) continue;
                if(eIdx>=0){
                    u.orderTarget=eIdx;
                    u.hasOrder=true; u.moving=true; u.action=ACT_MOVING;
                } else {
                    u.target={mouse.x+offX,mouse.y};
                    u.orderTarget=-1;
                    u.hasOrder=false; u.moving=true; u.action=ACT_MOVING;
                    offX=(offX>0)?(-offX):((-offX)+32); // zig-zag formation
                }
            }
        }
    }

    if(!g_gameOver){
        // ── UPDATE PLAYER UNITS ───────────────────────────────
        for(int i=0;i<(int)playerUnits.size();i++){
            BUnit& u=playerUnits[i];
            if(!u.alive) continue;
            if(u.typeIdx>=unitTypeCount()) continue;
            const UnitTypeDef& td=g_unitTypes[u.typeIdx];
            float radius=meleeRadius(td);

            u.meleeTimer-=dt;
            u.rangeTimer-=dt;

            // Explicit attack order
            Vector2 atkTarget=u.target;
            bool inRange=false;
            u.inMelee=false;

            if(u.orderTarget>=0&&u.orderTarget<(int)enemyUnits.size()
               &&enemyUnits[u.orderTarget].alive){
                BUnit& et=enemyUnits[u.orderTarget];
                const UnitTypeDef& etd=g_unitTypes[et.typeIdx];
                float etRadius=meleeRadius(etd);
                float dist=vdist(u.pos,et.pos);
                float meleeD=radius+etRadius+8.0f;
                u.angleTarget=dirToAngle(vnorm({et.pos.x-u.pos.x,et.pos.y-u.pos.y}));
                if(dist<=meleeD){
                    inRange=true; u.inMelee=true;
                    u.moving=false; u.action=ACT_ATTACKING;
                    if(u.meleeTimer<=0){
                        bool charge=u.chargeReady&&td.spriteBase==SPR_CAVALRY&&td.speed>=100;
                        float dmg=calcMeleeDmg(td,etd,u,charge);
                        if(charge&&dmg>0) u.chargeReady=false;
                        et.hp-=dmg;
                        if(et.hp<=0){et.alive=false;dead.push_back({et.pos,1.0f});}
                        u.meleeTimer=td.meleeInterval;
                    }
                } else if(td.range>0&&dist<=td.range&&!u.inMelee
                          &&losClean(u.pos,et.pos,playerUnits,i)){
                    // ranged attack
                    u.moving=false; u.action=ACT_ATTACKING;
                    if(u.rangeTimer<=0){
                        float dmg=calcMissileDmg(td,etd);
                        Vector2 dir2=vnorm({et.pos.x-u.pos.x,et.pos.y-u.pos.y});
                        projs.push_back({u.pos,{dir2.x*300,dir2.y*300},dmg,true,true});
                        u.rangeTimer=td.missileReload;
                    }
                } else {
                    // Move toward target
                    atkTarget=et.pos;
                    u.moving=true;
                }
            }

            // Auto-attack if no explicit order
            if(u.orderTarget<0||u.orderTarget>=(int)enemyUnits.size()||!enemyUnits[u.orderTarget].alive){
                u.orderTarget=-1; u.hasOrder=false;
                if(!u.moving||u.action==ACT_IDLE){
                    // find nearest enemy in auto range
                    float autoRange=(td.range>0)?(float)td.range:120.0f;
                    float best=autoRange; int bestIdx=-1;
                    for(int j=0;j<(int)enemyUnits.size();j++){
                        if(!enemyUnits[j].alive) continue;
                        float d=vdist(u.pos,enemyUnits[j].pos);
                        if(d<best){best=d;bestIdx=j;}
                    }
                    if(bestIdx>=0){
                        u.orderTarget=bestIdx;
                    }
                }
            }

            // Movement
            if(u.moving&&!inRange){
                Vector2 dir2=vnorm({atkTarget.x-u.pos.x,atkTarget.y-u.pos.y});
                float d=vdist(u.pos,atkTarget);
                if(d<4.0f){ u.moving=false; u.action=ACT_IDLE; }
                else{
                    u.pos.x+=dir2.x*td.speed*dt;
                    u.pos.y+=dir2.y*td.speed*dt;
                    u.angleTarget=dirToAngle(dir2);
                    u.action=ACT_MOVING;
                    u.stopTimer=0;
                }
            } else if(!inRange){
                u.stopTimer+=dt;
                if(u.stopTimer>1.0f) u.chargeReady=(td.spriteBase==SPR_CAVALRY);
                u.action=ACT_IDLE;
            }

            u.angle=lerpAngle(u.angle,u.angleTarget,10.0f*dt);
            // Clamp to battlefield
            u.pos.x=std::max(16.0f,std::min((float)(SCREEN_W-16),u.pos.x));
            u.pos.y=std::max(16.0f,std::min((float)(BATTLEFIELD_H-16),u.pos.y));
        }
        separateUnits(playerUnits);

        // ── UPDATE ENEMY AI ───────────────────────────────────
        for(int i=0;i<(int)enemyUnits.size();i++){
            BUnit& e=enemyUnits[i];
            if(!e.alive) continue;
            if(e.typeIdx>=unitTypeCount()) continue;
            const UnitTypeDef& etd=g_unitTypes[e.typeIdx];
            float radius=meleeRadius(etd);
            e.meleeTimer-=dt;
            e.rangeTimer-=dt;
            e.inMelee=false;

            // Find target based on difficulty
            BUnit* tgt=nullptr;
            float bestScore=1e9f;
            for(auto& pu:playerUnits){
                if(!pu.alive) continue;
                float d=vdist(e.pos,pu.pos);
                float score=d;
                if(cfg.difficulty==DIFF_HARD) score=(pu.hp/pu.maxHp)*10000.0f+d*0.1f; // weakest
                if(cfg.difficulty==DIFF_EASY&&e.orderTarget<0) continue; // passive
                if(score<bestScore){bestScore=score;tgt=&pu;}
            }
            // Easy: only attack if was attacked (simplified: always attack if in range)
            if(cfg.difficulty==DIFF_EASY){
                tgt=nullptr;
                float autoRange=(etd.range>0)?(float)etd.range:80.0f;
                float best=autoRange;
                for(auto& pu:playerUnits){
                    if(!pu.alive) continue;
                    float d=vdist(e.pos,pu.pos);
                    if(d<best){best=d;tgt=&pu;}
                }
            }

            if(tgt){
                const UnitTypeDef& ptd=g_unitTypes[tgt->typeIdx];
                float pRadius=meleeRadius(ptd);
                float dist=vdist(e.pos,tgt->pos);
                float meleeD=radius+pRadius+8.0f;
                Vector2 dir2=vnorm({tgt->pos.x-e.pos.x,tgt->pos.y-e.pos.y});
                e.angleTarget=dirToAngle(dir2);

                // Hard AI kite: if close melee threat and ranged, sidestep
                if(cfg.difficulty==DIFF_HARD&&etd.range>0&&dist<80.0f&&!e.inMelee){
                    Vector2 perp={-dir2.y,dir2.x};
                    e.pos.x+=perp.x*60.0f*dt;
                    e.pos.y+=perp.y*60.0f*dt;
                }

                if(dist<=meleeD){
                    e.inMelee=true; e.action=ACT_ATTACKING;
                    if(e.meleeTimer<=0){
                        bool charge=e.chargeReady&&etd.spriteBase==SPR_CAVALRY&&etd.speed>=100;
                        float dmg=calcMeleeDmg(etd,ptd,e,charge);
                        if(charge&&dmg>0) e.chargeReady=false;
                        tgt->hp-=dmg;
                        if(tgt->hp<=0){tgt->alive=false;dead.push_back({tgt->pos,1.0f});}
                        e.meleeTimer=etd.meleeInterval;
                    }
                } else if(etd.range>0&&dist<=(float)etd.range&&!e.inMelee
                          &&losClean(e.pos,tgt->pos,enemyUnits,i)){
                    e.action=ACT_ATTACKING;
                    if(e.rangeTimer<=0){
                        float dmg=calcMissileDmg(etd,ptd);
                        projs.push_back({e.pos,{dir2.x*280,dir2.y*280},dmg,true,false});
                        e.rangeTimer=etd.missileReload;
                    }
                } else {
                    e.pos.x+=dir2.x*etd.speed*dt;
                    e.pos.y+=dir2.y*etd.speed*dt;
                    e.action=ACT_MOVING;
                }
            } else { e.action=ACT_IDLE; }

            e.angle=lerpAngle(e.angle,e.angleTarget,8.0f*dt);
            e.pos.x=std::max(16.0f,std::min((float)(SCREEN_W-16),e.pos.x));
            e.pos.y=std::max(16.0f,std::min((float)(BATTLEFIELD_H-16),e.pos.y));
        }
        separateUnits(enemyUnits);

        // ── UPDATE PROJECTILES ────────────────────────────────
        for(auto& p:projs){
            if(!p.alive) continue;
            p.pos.x+=p.vel.x*dt; p.pos.y+=p.vel.y*dt;
            if(p.pos.x<0||p.pos.x>SCREEN_W||p.pos.y<0||p.pos.y>BATTLEFIELD_H){p.alive=false;continue;}
            if(p.fromPlayer){
                for(auto& e:enemyUnits){
                    if(!e.alive||!p.alive) continue;
                    if(vdist(p.pos,e.pos)<16){e.hp-=p.damage;p.alive=false;
                        if(e.hp<=0){e.alive=false;dead.push_back({e.pos,1.0f});}break;}
                }
            } else {
                for(auto& u:playerUnits){
                    if(!u.alive||!p.alive) continue;
                    if(vdist(p.pos,u.pos)<16){u.hp-=p.damage;p.alive=false;
                        if(u.hp<=0){u.alive=false;dead.push_back({u.pos,1.0f});}break;}
                }
            }
        }

        // Fade dead markers
        for(auto& d2:dead) d2.alpha-=dt*0.3f;
        dead.erase(std::remove_if(dead.begin(),dead.end(),[](const DeadMarker& d2){return d2.alpha<=0;}),dead.end());

        // Check win/lose
        int pa=0,ea=0;
        for(auto& u:playerUnits) if(u.alive) pa++;
        for(auto& e:enemyUnits) if(e.alive) ea++;
        if(pa==0){g_gameOver=true;g_victory=false;}
        if(ea==0&&pa>0){g_gameOver=true;g_victory=true;}
    }

    // ═══════════════════════════════════════════════════════════
    //  DRAW
    // ═══════════════════════════════════════════════════════════
    ClearBackground({22,32,18,255});
    // Grid
    for(int x2=0;x2<SCREEN_W;x2+=64) DrawLine(x2,0,x2,BATTLEFIELD_H,{30,48,24,255});
    for(int y2=0;y2<BATTLEFIELD_H;y2+=64) DrawLine(0,y2,SCREEN_W,y2,{30,48,24,255});
    // Center divide
    DrawLine(SCREEN_W/2,0,SCREEN_W/2,BATTLEFIELD_H,{50,80,40,80});

    // Dead markers
    for(auto& d2:dead){
        int a=(int)(d2.alpha*180);
        DrawLine((int)(d2.pos.x-10),(int)(d2.pos.y-10),(int)(d2.pos.x+10),(int)(d2.pos.y+10),{80,30,30,(unsigned char)a});
        DrawLine((int)(d2.pos.x+10),(int)(d2.pos.y-10),(int)(d2.pos.x-10),(int)(d2.pos.y+10),{80,30,30,(unsigned char)a});
    }

    // Projectiles
    for(auto& p:projs){
        if(!p.alive) continue;
        DrawCircleV(p.pos,4,p.fromPlayer?YELLOW:ORANGE);
    }

    // Draw order lines for selected
    for(auto& u:playerUnits){
        if(!u.selected||!u.alive) continue;
        if(u.orderTarget>=0&&u.orderTarget<(int)enemyUnits.size()&&enemyUnits[u.orderTarget].alive){
            DrawLineEx(u.pos,enemyUnits[u.orderTarget].pos,1,{255,100,100,120});
        } else if(u.moving){
            DrawLineEx(u.pos,u.target,1,{100,200,255,120});
        }
    }

    // Player units
    for(int i=0;i<(int)playerUnits.size();i++){
        const BUnit& u=playerUnits[i];
        if(!u.alive) continue;
        if(u.typeIdx>=unitTypeCount()) continue;
        const UnitTypeDef& td=g_unitTypes[u.typeIdx];
        if(u.selected){
            DrawCircleLines((int)u.pos.x,(int)u.pos.y,28,SKYBLUE);
            DrawCircle((int)u.pos.x,(int)u.pos.y,28,{100,200,255,40});
        }
        Color tint={td.r,td.g,td.b,255};
        if(u.selected) tint={255,255,255,255};
        drawSprite(g_playerTextures[u.typeIdx],u.pos,u.angle,1.8f,tint);
        drawHealthBar(u.pos,u.hp,u.maxHp,30);
        // Entities remaining
        int entLeft=(int)ceilf(u.hp/(float)td.hpPerEntity);
        DrawText(TextFormat("x%d",entLeft),(int)(u.pos.x+16),(int)(u.pos.y-20),10,{220,220,220,200});
    }

    // Enemy units
    for(auto& e:enemyUnits){
        if(!e.alive) continue;
        if(e.typeIdx>=unitTypeCount()) continue;
        const UnitTypeDef& etd=g_unitTypes[e.typeIdx];
        drawSprite(g_enemyTextures[e.typeIdx],e.pos,e.angle,1.8f,WHITE);
        drawHealthBar(e.pos,e.hp,e.maxHp,30);
        int entLeft=(int)ceilf(e.hp/(float)etd.hpPerEntity);
        DrawText(TextFormat("x%d",entLeft),(int)(e.pos.x+16),(int)(e.pos.y-20),10,{220,160,160,200});
    }

    // Selection box
    if(dragging&&(selRect.width>5||selRect.height>5)){
        DrawRectangleRec(selRect,{100,200,255,35});
        DrawRectangleLinesEx(selRect,1,SKYBLUE);
    }

    // ── HUD ──────────────────────────────────────────────────
    DrawRectangle(0,HUD_Y,SCREEN_W,SCREEN_H-HUD_Y,{5,8,5,240});
    DrawLine(0,HUD_Y,SCREEN_W,HUD_Y,{60,100,50,200});

    // Selected unit info (left)
    BUnit* sel=nullptr;
    for(auto& u:playerUnits) if(u.alive&&u.selected){sel=&u;break;}
    if(sel&&sel->typeIdx<unitTypeCount()){
        const UnitTypeDef& td=g_unitTypes[sel->typeIdx];
        Color pc={td.r,td.g,td.b,255};
        DrawRectangle(8,HUD_Y+6,46,46,pc);
        DrawRectangleLinesEx({8,(float)(HUD_Y+6),46,46},2,WHITE);
        DrawText(td.name,60,HUD_Y+8,14,WHITE);
        // HP bar
        float hpRatio=sel->hp/sel->maxHp;
        DrawRectangle(60,HUD_Y+26,220,12,DARKGRAY);
        DrawRectangle(60,HUD_Y+26,(int)(220*hpRatio),12,hpRatio>0.5f?GREEN:hpRatio>0.25f?YELLOW:RED);
        DrawText(TextFormat("%.0f / %.0f",sel->hp,sel->maxHp),286,HUD_Y+26,12,WHITE);
        int entLeft=(int)ceilf(sel->hp/(float)td.hpPerEntity);
        DrawText(TextFormat("Entities: %d / %d",entLeft,td.entities),60,HUD_Y+42,12,LIGHTGRAY);
        // Action
        const char* actLabels[]={"Idle","Moving","Attacking","Routing"};
        DrawText(actLabels[sel->action],290,HUD_Y+44,12,YELLOW);
    } else {
        DrawText("No unit selected",12,HUD_Y+34,14,{80,80,80,255});
    }

    // Kill counter (right)
    int pa=0,ea=0;
    for(auto& u:playerUnits) if(u.alive) pa++;
    for(auto& e:enemyUnits) if(e.alive) ea++;
    int pKills=0,eKills=0;
    for(auto& u:playerUnits) if(!u.alive) pKills++;  // rough proxy
    for(auto& e:enemyUnits) if(!e.alive) eKills++;
    DrawText(TextFormat("Allies: %d remaining",pa),(int)(SCREEN_W-420),HUD_Y+12,14,{80,140,255,255});
    DrawText(TextFormat("Enemies: %d remaining",ea),(int)(SCREEN_W-420),HUD_Y+32,14,{255,100,100,255});
    DrawText(TextFormat("Killed: %d",eKills),(int)(SCREEN_W-420),HUD_Y+52,12,{180,180,80,255});

    // Menu button (top right)
    if(drawButton({(float)(SCREEN_W-134),4,126,32},"MENU",mouse,{30,50,30,200},{60,110,60,255})){
        g_paused=true;
    }

    // ── GAME OVER / VICTORY OVERLAY ──────────────────────────
    if(g_gameOver){
        DrawRectangle(0,0,SCREEN_W,SCREEN_H,{0,0,0,160});
        if(g_victory){
            const char* w="VICTORY!";
            int tw2=MeasureText(w,60);
            DrawText(w,SCREEN_W/2-tw2/2+2,SCREEN_H/2-52+2,60,{0,100,0,255});
            DrawText(w,SCREEN_W/2-tw2/2,  SCREEN_H/2-52,  60,GREEN);
        } else {
            const char* w="DEFEAT!";
            int tw2=MeasureText(w,60);
            DrawText(w,SCREEN_W/2-tw2/2+2,SCREEN_H/2-52+2,60,{130,0,0,255});
            DrawText(w,SCREEN_W/2-tw2/2,  SCREEN_H/2-52,  60,RED);
        }
        if(drawButton({(float)(SCREEN_W/2-140),(float)(SCREEN_H/2+20),280,50},"RETURN TO MENU",mouse)){
            g_gameOver=false; return STATE_MENU;
        }
    }

    return STATE_PLAYING;
}

// ═══════════════════════════════════════════════════════════════
//  MAIN
// ═══════════════════════════════════════════════════════════════
int main(){
    InitWindow(SCREEN_W,SCREEN_H,"Medieval Warfare");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    initBuiltinTypes();
    rebuildPreviewTex();

    GameState    state=STATE_MENU;
    BattleConfig cfg  =defaultBattleConfig();

    std::vector<BUnit>      playerUnits, enemyUnits;
    std::vector<Projectile> projs;
    std::vector<DeadMarker> dead;
    float menuTime=0;

    while(!WindowShouldClose()&&!g_quitRequested){
        if(IsKeyPressed(KEY_F11)||(IsKeyDown(KEY_LEFT_ALT)&&IsKeyPressed(KEY_ENTER)))
            ToggleFullscreen();
        SCREEN_W=GetScreenWidth();
        SCREEN_H=GetScreenHeight();

        float dt=GetFrameTime();
        menuTime+=dt;
        Vector2 mouse=GetMousePosition();

        BeginDrawing();
        switch(state){
            case STATE_MENU:{
                GameState nxt=updateDrawMenu(mouse,menuTime);
                if(nxt==STATE_CUSTOM_BATTLE){
                    resizeBattleConfig(cfg);
                    state=nxt;
                } else if(nxt==STATE_UNIT_EDITOR){
                    state=nxt;
                } else {
                    state=nxt;
                }
                break;
            }
            case STATE_CUSTOM_BATTLE:{
                GameState nxt=updateDrawCustomBattle(cfg,mouse);
                if(nxt==STATE_PLAYING){
                    resizeBattleConfig(cfg);
                    initGame(playerUnits,enemyUnits,projs,dead,cfg);
                    g_gameOver=false; g_victory=false; g_paused=false;
                    state=STATE_PLAYING;
                } else {
                    state=nxt;
                }
                break;
            }
            case STATE_UNIT_EDITOR:{
                state=updateDrawUnitEditor(mouse,dt);
                break;
            }
            case STATE_PLAYING:{
                state=updateDrawGame(playerUnits,enemyUnits,projs,dead,cfg,mouse,dt);
                break;
            }
        }
        EndDrawing();
    }

    // Cleanup
    for(auto& t:g_playerTextures) UnloadTexture(t);
    for(auto& t:g_enemyTextures)  UnloadTexture(t);
    if(g_previewTex.id>0) UnloadTexture(g_previewTex);
    if(g_draftTex.id>0) UnloadTexture(g_draftTex);
    CloseWindow();
    return 0;
}
