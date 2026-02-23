#include "raylib.h"
#include <vector>
#include <cmath>
#include <string>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <ctime>

// ═══════════════════════════════════════════════════════════════
//  SCREEN
// ═══════════════════════════════════════════════════════════════
int SCREEN_W = 1280;
int SCREEN_H = 768;

// ═══════════════════════════════════════════════════════════════
//  UTILS
// ═══════════════════════════════════════════════════════════════
unsigned char clampU8(int v){ return (unsigned char)std::max(0,std::min(255,v)); }
float vdist(Vector2 a,Vector2 b){float dx=a.x-b.x,dy=a.y-b.y;return sqrtf(dx*dx+dy*dy);}
Vector2 vnorm(Vector2 v){float l=sqrtf(v.x*v.x+v.y*v.y);if(l<0.001f)return{0,-1};return{v.x/l,v.y/l};}
float dirToAngle(Vector2 d){return atan2f(d.x,-d.y)*RAD2DEG;}
float lerpAngle(float c,float t,float s){
    float d=t-c; while(d>180)d-=360; while(d<-180)d+=360; return c+d*s;
}
float lerp(float a,float b,float t){return a+(b-a)*t;}
bool btnHover(Rectangle r,Vector2 m){return CheckCollisionPointRec(m,r);}
int randRange(int lo,int hi){return lo+(rand()%(hi-lo+1));}
float frandRange(float lo,float hi){return lo+(float)rand()/(float)RAND_MAX*(hi-lo);}

static void px(Image& img,int x,int y,Color c){
    if(x<0||y<0||x>=img.width||y>=img.height) return;
    ImageDrawPixel(&img,x,y,c);
}
static void pxRect(Image& img,int x,int y,int w,int h,Color c){
    for(int dy=0;dy<h;dy++) for(int dx=0;dx<w;dx++) px(img,x+dx,y+dy,c);
}

// ═══════════════════════════════════════════════════════════════
//  UNIT TYPE ENUM
// ═══════════════════════════════════════════════════════════════
enum UnitRole { ROLE_LIGHT_INF=0, ROLE_HEAVY_INF, ROLE_RANGED, ROLE_LIGHT_CAV, ROLE_HEAVY_CAV, ROLE_COUNT };
const char* roleNames[ROLE_COUNT]={"Light Infantry","Heavy Infantry","Ranged Infantry","Light Cavalry","Heavy Cavalry"};

// ═══════════════════════════════════════════════════════════════
//  SPRITE GENERATION  (16x16 top-down pixel art)
// ═══════════════════════════════════════════════════════════════
// Colors
static Color C_SKIN  = {220,170,120,255};
static Color C_DARK  = {40,30,20,255};
static Color C_BLOND = {210,180,80,255};
static Color C_CLOTH = {180,160,120,255};
static Color C_PADDED= {150,130,90,255};
static Color C_CHAIN = {140,140,150,255};
static Color C_PLATE = {200,200,210,255};
static Color C_BRIGANDINE = {120,100,70,255};
static Color C_SHIELD_WOOD= {140,90,50,255};
static Color C_SHIELD_METAL={160,160,170,255};
static Color C_WEAPON_METAL={200,200,215,255};
static Color C_HORSE_BROWN ={120,75,40,255};
static Color C_HORSE_BLACK ={50,40,35,255};
static Color C_BLOOD  = {180,20,20,200};

// Draw a top-down soldier figure in a 16x16 image
// armorCol = body color, helmCol = helmet, weaponCol = weapon color
// hasShield, hasPolearm, hasRanged, hasCav
Image makeUnitSprite(Color armorCol, Color helmCol, Color weaponCol,
                     bool hasShield, bool isPolearm, bool isCav,
                     Color horseCol={120,75,40,255})
{
    Image img = GenImageColor(16,16,BLANK);

    if(isCav){
        // Horse body (oval, top-down)
        Color hc = horseCol;
        Color hd = {clampU8(hc.r-40),clampU8(hc.g-30),clampU8(hc.b-20),255};
        for(int y=5;y<=12;y++) for(int x=3;x<=13;x++){
            float nx=(x-8)/5.0f, ny=(y-8.5f)/3.5f;
            if(nx*nx+ny*ny<=1.0f) px(img,x,y,hc);
        }
        // Horse legs (4 dots at corners)
        px(img,4,12,hd); px(img,5,13,hd);
        px(img,11,12,hd); px(img,10,13,hd);
        px(img,4,6,hd); px(img,5,5,hd);
        px(img,11,6,hd); px(img,10,5,hd);
        // Rider torso (center top)
        pxRect(img,6,4,4,5,armorCol);
        px(img,7,4,helmCol); px(img,8,4,helmCol);
        px(img,7,3,helmCol); px(img,8,3,helmCol);
        // Rider head
        px(img,7,2,C_SKIN); px(img,8,2,C_SKIN);
        // Weapon
        if(isPolearm){
            for(int yy=0;yy<=5;yy++) px(img,10,yy,weaponCol);
            px(img,10,0,{220,180,60,255});
        } else {
            px(img,10,3,weaponCol); px(img,10,2,weaponCol); px(img,11,2,weaponCol);
        }
        if(hasShield){
            pxRect(img,5,3,3,4,C_SHIELD_METAL);
            px(img,5,3,{200,200,200,255});
        }
    } else {
        // Top-down infantry
        // Body/torso center
        pxRect(img,5,5,6,7,armorCol);
        // Shoulder pads
        px(img,4,5,armorCol); px(img,11,5,armorCol);
        px(img,4,6,armorCol); px(img,11,6,armorCol);
        // Helm / head
        pxRect(img,6,2,4,3,helmCol);
        px(img,6,3,{clampU8(helmCol.r-30),clampU8(helmCol.g-30),clampU8(helmCol.b-30),255});
        px(img,9,3,{clampU8(helmCol.r-30),clampU8(helmCol.g-30),clampU8(helmCol.b-30),255});
        // Face/visor slot
        px(img,7,2,C_SKIN); px(img,8,2,C_SKIN);
        // Legs
        px(img,6,12,armorCol); px(img,7,12,armorCol);
        px(img,8,12,armorCol); px(img,9,12,armorCol);
        px(img,6,13,C_DARK);  px(img,9,13,C_DARK);
        // Weapon
        if(isPolearm){
            // Polearm on right side
            for(int yy=0;yy<=13;yy++) px(img,12,yy,weaponCol);
            px(img,12,0,{220,180,60,255}); px(img,11,0,{220,180,60,255});
        } else {
            // Sword/axe
            px(img,11,4,weaponCol); px(img,12,3,weaponCol);
            px(img,11,3,weaponCol); px(img,12,4,weaponCol);
            px(img,10,5,weaponCol);
        }
        // Shield
        if(hasShield){
            pxRect(img,2,5,4,5,C_SHIELD_WOOD);
            px(img,3,6,{clampU8(C_SHIELD_WOOD.r+30),clampU8(C_SHIELD_WOOD.g+20),clampU8(C_SHIELD_WOOD.b+10),255});
        }
    }
    return img;
}

Image makeRangedSprite(Color armorCol, Color helmCol, bool isCrossbow){
    Image img = GenImageColor(16,16,BLANK);
    // Body
    pxRect(img,5,5,6,7,armorCol);
    px(img,4,5,armorCol); px(img,11,5,armorCol);
    // Head
    pxRect(img,6,2,4,3,helmCol);
    px(img,7,2,C_SKIN); px(img,8,2,C_SKIN);
    // Legs
    px(img,6,12,armorCol); px(img,7,12,armorCol);
    px(img,8,12,armorCol); px(img,9,12,armorCol);
    px(img,6,13,C_DARK); px(img,9,13,C_DARK);
    if(isCrossbow){
        // Crossbow horizontal
        for(int xx=2;xx<=13;xx++) px(img,xx,7,{100,70,40,255});
        px(img,7,6,{200,200,210,255}); px(img,8,6,{200,200,210,255});
        px(img,7,8,{200,200,210,255}); px(img,8,8,{200,200,210,255});
    } else {
        // Bow (arc on left)
        for(int yy=2;yy<=12;yy++) px(img,3,yy,{100,70,40,255});
        px(img,4,2,{100,70,40,255}); px(img,4,12,{100,70,40,255});
        // Arrow
        px(img,5,7,{180,150,60,255}); px(img,6,7,{180,150,60,255});
        px(img,7,7,{200,200,210,255});
    }
    return img;
}

Texture2D imgToTex(Image img){
    Texture2D tex=LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(tex,TEXTURE_FILTER_POINT);
    return tex;
}

// ═══════════════════════════════════════════════════════════════
//  UNIT TYPE DEFINITION
// ═══════════════════════════════════════════════════════════════
struct UnitTypeDef {
    char  name[48];
    UnitRole role;
    int   numEntities;
    int   cost;
    float healthPerEntity;
    float armour;            // 0-100
    float speed;
    int   meleeAttack;
    float meleeAttackInterval;
    int   meleeDefense;
    float meleeBaseDmg;
    float meleeAPDmg;
    float range;             // 0 = no ranged
    float missileBaseDmg;
    float missileAPDmg;
    float missileReloadTime;
    bool  isBuiltin;
    // Sprite colors (for display/editor)
    unsigned char r,g,b;
    unsigned char hr,hg,hb; // helmet color
    bool hasShield, isPolearm, isCav, isCrossbow;
    Color horseCol;
};

static std::vector<UnitTypeDef> g_unitTypes;
static std::vector<Texture2D>   g_playerTex;
static std::vector<Texture2D>   g_enemyTex;

void rebuildTexture(int idx){
    const UnitTypeDef& td = g_unitTypes[idx];
    Color ac={td.r,td.g,td.b,255};
    Color hc={td.hr,td.hg,td.hb,255};
    Color wc=C_WEAPON_METAL;

    Image img;
    if(td.range>0 && td.role==ROLE_RANGED)
        img=makeRangedSprite(ac,hc,td.isCrossbow);
    else
        img=makeUnitSprite(ac,hc,wc,td.hasShield,td.isPolearm,td.isCav,td.horseCol);

    if(idx<(int)g_playerTex.size()) UnloadTexture(g_playerTex[idx]);
    if(idx<(int)g_playerTex.size()) g_playerTex[idx]=imgToTex(img);
    else g_playerTex.push_back(imgToTex(img));

    // Enemy = reddened
    unsigned char er=clampU8((int)(td.r*0.4f+160*0.6f));
    unsigned char eg=clampU8((int)(td.g*0.3f));
    unsigned char eb=clampU8((int)(td.b*0.3f));
    Color eac={er,eg,eb,255};
    Color ehc={clampU8(td.hr/2+80),clampU8(td.hg/3),clampU8(td.hb/3),255};

    Image eimg;
    if(td.range>0 && td.role==ROLE_RANGED)
        eimg=makeRangedSprite(eac,ehc,td.isCrossbow);
    else
        eimg=makeUnitSprite(eac,ehc,wc,td.hasShield,td.isPolearm,td.isCav,td.horseCol);

    if(idx<(int)g_enemyTex.size()) UnloadTexture(g_enemyTex[idx]);
    if(idx<(int)g_enemyTex.size()) g_enemyTex[idx]=imgToTex(eimg);
    else g_enemyTex.push_back(imgToTex(eimg));
}

void initBuiltinTypes(){
    g_unitTypes.clear();

    // 0: Spear Levy
    UnitTypeDef t={};
    strcpy(t.name,"Spear Levy");
    t.role=ROLE_LIGHT_INF; t.numEntities=60; t.cost=100;
    t.healthPerEntity=30; t.armour=0; t.speed=80;
    t.meleeAttack=10; t.meleeAttackInterval=1.5f; t.meleeDefense=5;
    t.meleeBaseDmg=8; t.meleeAPDmg=2;
    t.range=0; t.isBuiltin=true;
    t.r=180; t.g=160; t.b=120;    // cloth
    t.hr=180; t.hg=160; t.hb=120; // no helm
    t.hasShield=false; t.isPolearm=true; t.isCav=false;
    g_unitTypes.push_back(t);

    // 1: Axe Militia
    memset(&t,0,sizeof(t));
    strcpy(t.name,"Axe Militia");
    t.role=ROLE_LIGHT_INF; t.numEntities=50; t.cost=150;
    t.healthPerEntity=40; t.armour=10; t.speed=90;
    t.meleeAttack=18; t.meleeAttackInterval=1.2f; t.meleeDefense=12;
    t.meleeBaseDmg=14; t.meleeAPDmg=4;
    t.range=0; t.isBuiltin=true;
    t.r=150; t.g=130; t.b=90;    // padded
    t.hr=80; t.hg=70; t.hb=60;  // simple cap
    t.hasShield=true; t.isPolearm=false; t.isCav=false;
    g_unitTypes.push_back(t);

    // 2: Poleaxe Retainers
    memset(&t,0,sizeof(t));
    strcpy(t.name,"Poleaxe Retainers");
    t.role=ROLE_HEAVY_INF; t.numEntities=40; t.cost=300;
    t.healthPerEntity=60; t.armour=30; t.speed=70;
    t.meleeAttack=28; t.meleeAttackInterval=1.4f; t.meleeDefense=20;
    t.meleeBaseDmg=18; t.meleeAPDmg=10;
    t.range=0; t.isBuiltin=true;
    t.r=140; t.g=140; t.b=150;  // chainmail
    t.hr=120; t.hg=120; t.hb=130;
    t.hasShield=false; t.isPolearm=true; t.isCav=false;
    g_unitTypes.push_back(t);

    // 3: Dismounted Knights
    memset(&t,0,sizeof(t));
    strcpy(t.name,"Dismounted Knights");
    t.role=ROLE_HEAVY_INF; t.numEntities=24; t.cost=500;
    t.healthPerEntity=80; t.armour=60; t.speed=55;
    t.meleeAttack=38; t.meleeAttackInterval=1.6f; t.meleeDefense=30;
    t.meleeBaseDmg=20; t.meleeAPDmg=18;
    t.range=0; t.isBuiltin=true;
    t.r=200; t.g=200; t.b=210;  // full plate
    t.hr=210; t.hg=210; t.hb=220;
    t.hasShield=false; t.isPolearm=false; t.isCav=false;
    g_unitTypes.push_back(t);

    // 4: Bow Levy
    memset(&t,0,sizeof(t));
    strcpy(t.name,"Bow Levy");
    t.role=ROLE_RANGED; t.numEntities=50; t.cost=120;
    t.healthPerEntity=25; t.armour=0; t.speed=85;
    t.meleeAttack=8; t.meleeAttackInterval=1.5f; t.meleeDefense=4;
    t.meleeBaseDmg=6; t.meleeAPDmg=1;
    t.range=200; t.missileBaseDmg=10; t.missileAPDmg=2; t.missileReloadTime=2.0f;
    t.isBuiltin=true;
    t.r=180; t.g=155; t.b=110;
    t.hr=160; t.hg=140; t.hb=100;
    t.hasShield=false; t.isPolearm=false; t.isCav=false; t.isCrossbow=false;
    g_unitTypes.push_back(t);

    // 5: Crossbow Retainers
    memset(&t,0,sizeof(t));
    strcpy(t.name,"Crossbow Retainers");
    t.role=ROLE_RANGED; t.numEntities=36; t.cost=280;
    t.healthPerEntity=45; t.armour=20; t.speed=65;
    t.meleeAttack=15; t.meleeAttackInterval=1.3f; t.meleeDefense=14;
    t.meleeBaseDmg=10; t.meleeAPDmg=5;
    t.range=180; t.missileBaseDmg=14; t.missileAPDmg=8; t.missileReloadTime=3.5f;
    t.isBuiltin=true;
    t.r=120; t.g=100; t.b=70;  // brigandine
    t.hr=100; t.hg=90; t.hb=75;
    t.hasShield=false; t.isPolearm=false; t.isCav=false; t.isCrossbow=true;
    g_unitTypes.push_back(t);

    // 6: Mounted Sergeants
    memset(&t,0,sizeof(t));
    strcpy(t.name,"Mounted Sergeants");
    t.role=ROLE_LIGHT_CAV; t.numEntities=30; t.cost=350;
    t.healthPerEntity=50; t.armour=20; t.speed=160;
    t.meleeAttack=25; t.meleeAttackInterval=1.1f; t.meleeDefense=18;
    t.meleeBaseDmg=16; t.meleeAPDmg=6;
    t.range=0; t.isBuiltin=true;
    t.r=120; t.g=100; t.b=70;
    t.hr=100; t.hg=90; t.hb=75;
    t.hasShield=true; t.isPolearm=true; t.isCav=true;
    t.horseCol={140,90,55,255};
    g_unitTypes.push_back(t);

    // 7: Knights (Poleaxes)
    memset(&t,0,sizeof(t));
    strcpy(t.name,"Knights");
    t.role=ROLE_HEAVY_CAV; t.numEntities=16; t.cost=700;
    t.healthPerEntity=100; t.armour=70; t.speed=200;
    t.meleeAttack=42; t.meleeAttackInterval=1.5f; t.meleeDefense=28;
    t.meleeBaseDmg=22; t.meleeAPDmg=20;
    t.range=0; t.isBuiltin=true;
    t.r=210; t.g=210; t.b=220;
    t.hr=215; t.hg=215; t.hb=225;
    t.hasShield=false; t.isPolearm=true; t.isCav=true;
    t.horseCol={60,50,45,255};
    g_unitTypes.push_back(t);

    g_playerTex.clear(); g_enemyTex.clear();
    for(int i=0;i<(int)g_unitTypes.size();i++) rebuildTexture(i);
}

int unitTypeCount(){ return (int)g_unitTypes.size(); }

// ═══════════════════════════════════════════════════════════════
//  STATES
// ═══════════════════════════════════════════════════════════════
enum GameState { STATE_MENU, STATE_CUSTOM_BATTLE, STATE_UNIT_EDITOR, STATE_PLAYING };

// ═══════════════════════════════════════════════════════════════
//  BATTLE CONFIG
// ═══════════════════════════════════════════════════════════════
struct BattleConfig {
    std::vector<int> playerCounts;
    std::vector<int> enemyCounts;
    float difficulty;
};

BattleConfig defaultBattleConfig(){
    BattleConfig c;
    int n=unitTypeCount();
    c.playerCounts.assign(n,0);
    c.enemyCounts.assign(n,0);
    if(n>0){ c.playerCounts[0]=1; c.enemyCounts[0]=1; }
    c.difficulty=1.0f;
    return c;
}

void resizeBattleConfig(BattleConfig& c){
    int n=unitTypeCount();
    c.playerCounts.resize(n,0);
    c.enemyCounts.resize(n,0);
}

// ═══════════════════════════════════════════════════════════════
//  GAME ENTITIES
// ═══════════════════════════════════════════════════════════════
struct Entity {
    Vector2 pos;
    float   hp;
    bool    alive;
};

struct BattleUnit {
    int     typeIdx;
    bool    isPlayer;
    bool    selected;

    Vector2 pos;        // center of the unit formation
    Vector2 target;     // movement target
    float   angle;      // facing angle in degrees
    float   angleTarget;

    std::vector<Entity> entities; // individual soldiers
    float   missileReloadTimer;
    float   meleeAttackTimer;

    // State
    bool    moving;
    bool    inMeleeWith;   // dummy flag
    int     targetUnitIdx; // index into battle units array (-1 = none)
    bool    forceAttack;   // right-clicked on enemy
};

struct Projectile {
    Vector2 pos, vel;
    float   damage, apDamage;
    bool    alive, fromPlayer;
    int     targetUnitIdx;
};

// ═══════════════════════════════════════════════════════════════
//  UI HELPERS
// ═══════════════════════════════════════════════════════════════
bool drawButton(Rectangle r,const char* lbl,Vector2 m,
                Color cn={40,60,40,255},Color ch={70,120,60,255}){
    bool hv=btnHover(r,m);
    DrawRectangleRec(r,hv?ch:cn);
    DrawRectangleLinesEx(r,2,hv?Color{220,200,140,255}:Color{120,100,60,255});
    int tw=MeasureText(lbl,18);
    DrawText(lbl,(int)(r.x+r.width/2-tw/2),(int)(r.y+r.height/2-9),18,hv?Color{255,240,200,255}:Color{200,185,140,255});
    return hv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

float drawSlider(Rectangle r,float val,float vmin,float vmax,
                 const char* label,const char* fmt,Vector2 mouse,
                 Color fill={140,100,40,200}){
    if(label&&label[0]) DrawText(label,(int)r.x,(int)(r.y-18),13,{200,185,140,255});
    DrawRectangleRec(r,{20,18,12,255});
    DrawRectangleLinesEx(r,1,{80,65,40,255});
    float t=(val-vmin)/(vmax-vmin);
    DrawRectangle((int)r.x,(int)r.y,(int)(t*r.width),(int)r.height,fill);
    float kx=r.x+t*r.width;
    DrawRectangle((int)(kx-5),(int)(r.y-3),10,(int)(r.height+6),{220,200,140,255});
    DrawText(TextFormat(fmt,val),(int)(r.x+r.width+8),(int)(r.y+1),13,{220,200,140,255});
    if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)&&CheckCollisionPointRec(mouse,r)){
        t=(mouse.x-r.x)/r.width; t=fmaxf(0,fminf(1,t));
        val=vmin+t*(vmax-vmin);
    }
    return val;
}

void drawHealthBar(Vector2 pos,float hp,float maxHp,float w){
    float ratio=hp/maxHp;
    DrawRectangle((int)(pos.x-w/2),(int)(pos.y-24),(int)w,5,{20,15,10,200});
    Color hcol=ratio>0.5f?Color{60,180,50,255}:ratio>0.25f?Color{200,180,30,255}:Color{200,50,30,255};
    DrawRectangle((int)(pos.x-w/2),(int)(pos.y-24),(int)(w*ratio),5,hcol);
}

void drawSprite(Texture2D tex,Vector2 pos,float angleDeg,float scale,Color tint){
    float s=16*scale;
    DrawTexturePro(tex,{0,0,16,16},{pos.x,pos.y,s,s},{s/2,s/2},-angleDeg+180.0f,tint);
}

// ═══════════════════════════════════════════════════════════════
//  MAIN MENU
// ═══════════════════════════════════════════════════════════════
GameState updateDrawMenu(Vector2 mouse, float t){
    // Parchment/dark medieval background
    ClearBackground({15,12,8,255});

    // Grid pattern
    for(int x=0;x<SCREEN_W;x+=48)
        DrawLine(x,0,x,SCREEN_H,{30,25,15,(unsigned char)(40+10*(int)sinf(t+x*0.02f))});
    for(int y=0;y<SCREEN_H;y+=48)
        DrawLine(0,y,SCREEN_W,y,{30,25,15,(unsigned char)(40+10*(int)sinf(t+y*0.02f))});

    // Decorative border
    DrawRectangleLinesEx({10,10,(float)SCREEN_W-20,(float)SCREEN_H-20},3,{120,95,45,180});
    DrawRectangleLinesEx({18,18,(float)SCREEN_W-36,(float)SCREEN_H-36},1,{90,70,30,120});

    // Corner ornaments
    auto corner=[&](int cx,int cy){
        DrawRectangle(cx-8,cy-2,16,4,{150,115,55,255});
        DrawRectangle(cx-2,cy-8,4,16,{150,115,55,255});
        DrawCircleLines(cx,cy,10,{120,90,40,180});
    };
    corner(20,20); corner(SCREEN_W-20,20);
    corner(20,SCREEN_H-20); corner(SCREEN_W-20,SCREEN_H-20);

    // Title banner
    DrawRectangle(SCREEN_W/2-320,80,640,90,{25,18,10,230});
    DrawRectangleLinesEx({(float)(SCREEN_W/2-320),80,640,90},2,{150,115,55,255});

    const char* title="MEDIEVAL WARFARE";
    int tw=MeasureText(title,52);
    DrawText(title,SCREEN_W/2-tw/2+2,92,52,{80,50,10,200});
    DrawText(title,SCREEN_W/2-tw/2,90,52,{220,185,100,255});

    const char* sub="A Total War in the Age of Knights";
    DrawText(sub,SCREEN_W/2-MeasureText(sub,16)/2,152,16,{160,130,70,220});

    // Buttons
    float bx=SCREEN_W/2-170.0f, bw=340, bh=56;
    if(drawButton({bx,260,bw,bh},"CUSTOM BATTLE",mouse,{35,25,12,255},{65,50,20,255})) return STATE_CUSTOM_BATTLE;
    if(drawButton({bx,330,bw,bh},"UNIT EDITOR",mouse,{35,25,12,255},{65,50,20,255}))   return STATE_UNIT_EDITOR;
    if(drawButton({bx,400,bw,bh},"EXIT GAME",mouse,{50,20,12,255},{90,35,18,255}))      return STATE_MENU; // handled in main

    // Controls hint
    const char* hint="LMB: Select Units  |  RMB: Move / Attack  |  Drag: Box Select  |  F11: Fullscreen";
    DrawText(hint,SCREEN_W/2-MeasureText(hint,12)/2,SCREEN_H-36,12,{100,80,40,200});

    return STATE_MENU;
}

// ═══════════════════════════════════════════════════════════════
//  CUSTOM BATTLE
// ═══════════════════════════════════════════════════════════════
static bool g_customInit=false;

GameState updateDrawCustomBattle(BattleConfig& cfg, Vector2 mouse){
    if(!g_customInit){ resizeBattleConfig(cfg); g_customInit=true; }

    ClearBackground({12,10,6,255});
    for(int x=0;x<SCREEN_W;x+=48) DrawLine(x,0,x,SCREEN_H,{28,22,12,50});
    for(int y=0;y<SCREEN_H;y+=48) DrawLine(0,y,SCREEN_W,y,{28,22,12,50});

    DrawRectangle(0,0,SCREEN_W,56,{8,6,3,240});
    DrawRectangleLinesEx({0,56,(float)SCREEN_W,1},1,{120,95,45,180});

    DrawText("DEPLOY YOUR FORCES",20,12,28,{220,185,100,255});
    DrawText("Choose how many units to deploy on each side",20,44,13,{140,115,60,200});

    int headerH=60, bottomH=64;
    int bodyY=headerH+4, bodyH=SCREEN_H-headerH-bottomH-8;
    int nc=unitTypeCount();
    float cardW=(SCREEN_W-40.0f)/nc;

    for(int i=0;i<nc;i++){
        const UnitTypeDef& td=g_unitTypes[i];
        float cx=20+i*cardW, cy=(float)bodyY;
        float cw=cardW-8, ch=(float)bodyH;

        Color roleBg={25,20,10,220};
        DrawRectangle((int)cx,(int)cy,(int)cw,(int)ch,roleBg);
        DrawRectangleLinesEx({cx,cy,cw,ch},1,{100,80,35,200});

        // Header with unit name
        Color roleCol={220,185,100,255};
        switch(td.role){
            case ROLE_HEAVY_INF: roleCol={180,100,60,255}; break;
            case ROLE_RANGED:    roleCol={100,180,120,255}; break;
            case ROLE_LIGHT_CAV: roleCol={100,140,220,255}; break;
            case ROLE_HEAVY_CAV: roleCol={200,160,220,255}; break;
            default: break;
        }
        DrawRectangle((int)cx,(int)cy,(int)cw,28,{20,15,8,220});
        int nw=MeasureText(td.name,13);
        DrawText(td.name,(int)(cx+cw/2-nw/2),(int)(cy+7),13,roleCol);

        // Sprite preview
        if(i<(int)g_playerTex.size())
            drawSprite(g_playerTex[i],{cx+cw/2,cy+60},0,3.5f,WHITE);

        // Stats
        float sy=cy+100;
        auto statLine=[&](const char* lbl,const char* val,Color vc){
            int lw=MeasureText(lbl,11);
            DrawText(lbl,(int)(cx+6),(int)sy,11,{150,130,80,255});
            DrawText(val,(int)(cx+cw-MeasureText(val,11)-6),(int)sy,11,vc);
            sy+=15;
        };
        statLine("Entities:", TextFormat("%d",td.numEntities),{200,200,200,255});
        statLine("HP/Entity:",TextFormat("%.0f",td.healthPerEntity),{60,220,60,255});
        statLine("Armour:",   TextFormat("%.0f",td.armour),{180,180,100,255});
        statLine("Speed:",    TextFormat("%.0f",td.speed),{100,180,220,255});
        statLine("Melee Atk:",TextFormat("%d",td.meleeAttack),{220,160,60,255});
        statLine("Melee Def:",TextFormat("%d",td.meleeDefense),{100,200,120,255});
        if(td.range>0){
            statLine("Range:",TextFormat("%.0f",td.range),{200,220,100,255});
            statLine("Miss. DMG:",TextFormat("%.0f+%.0fAP",td.missileBaseDmg,td.missileAPDmg),YELLOW);
        }
        statLine("Cost:",TextFormat("%dg",td.cost),{220,185,100,255});

        // Selector buttons
        sy=cy+ch-88;
        DrawText("PLAYER ARMY",(int)(cx+6),(int)sy,11,{100,160,240,255}); sy+=14;
        Rectangle rm={(cx+4),(sy),(cw-8),24};
        bool rmhv=CheckCollisionPointRec(mouse,rm);
        DrawRectangleRec(rm,rmhv?Color{40,60,100,255}:Color{20,35,65,220});
        DrawRectangleLinesEx(rm,1,rmhv?Color{100,150,255,255}:Color{50,80,150,255});
        int pCount=i<(int)cfg.playerCounts.size()?cfg.playerCounts[i]:0;
        // - button
        Rectangle pMinus={cx+4,sy,26,24};
        Rectangle pPlus={cx+cw-30,sy,26,24};
        Rectangle pLabel={cx+30,sy,cw-64,24};
        bool pmHv=CheckCollisionPointRec(mouse,pMinus);
        bool ppHv=CheckCollisionPointRec(mouse,pPlus);
        DrawRectangleRec(pMinus,pmHv?Color{120,60,30,255}:Color{70,35,15,220});
        DrawRectangleLinesEx(pMinus,1,{150,80,40,255});
        DrawText("-",(int)(pMinus.x+8),(int)(pMinus.y+4),16,{220,185,100,255});
        DrawRectangleRec(pPlus,ppHv?Color{30,90,40,255}:Color{15,50,20,220});
        DrawRectangleLinesEx(pPlus,1,{60,150,80,255});
        DrawText("+",(int)(pPlus.x+6),(int)(pPlus.y+4),16,{120,220,140,255});
        DrawRectangleRec(pLabel,{15,18,8,220});
        DrawText(TextFormat("%d",pCount),(int)(pLabel.x+pLabel.width/2-MeasureText(TextFormat("%d",pCount),14)/2),(int)(pLabel.y+4),14,WHITE);
        if(pmHv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)&&pCount>0) cfg.playerCounts[i]--;
        if(ppHv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) cfg.playerCounts[i]++;
        sy+=30;

        DrawText("ENEMY ARMY",(int)(cx+6),(int)sy,11,{220,100,80,255}); sy+=14;
        int eCount=i<(int)cfg.enemyCounts.size()?cfg.enemyCounts[i]:0;
        Rectangle eMinus={cx+4,sy,26,24};
        Rectangle ePlus={cx+cw-30,sy,26,24};
        Rectangle eLabel={cx+30,sy,cw-64,24};
        bool emHv=CheckCollisionPointRec(mouse,eMinus);
        bool epHv=CheckCollisionPointRec(mouse,ePlus);
        DrawRectangleRec(eMinus,emHv?Color{120,60,30,255}:Color{70,35,15,220});
        DrawRectangleLinesEx(eMinus,1,{150,80,40,255});
        DrawText("-",(int)(eMinus.x+8),(int)(eMinus.y+4),16,{220,185,100,255});
        DrawRectangleRec(ePlus,epHv?Color{120,30,30,255}:Color{70,15,15,220});
        DrawRectangleLinesEx(ePlus,1,{180,60,60,255});
        DrawText("+",(int)(ePlus.x+6),(int)(ePlus.y+4),16,{220,100,80,255});
        DrawRectangleRec(eLabel,{20,8,8,220});
        DrawText(TextFormat("%d",eCount),(int)(eLabel.x+eLabel.width/2-MeasureText(TextFormat("%d",eCount),14)/2),(int)(eLabel.y+4),14,WHITE);
        if(emHv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)&&eCount>0) cfg.enemyCounts[i]--;
        if(epHv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) cfg.enemyCounts[i]++;
    }

    // Bottom bar
    int barY=SCREEN_H-bottomH;
    DrawRectangle(0,barY,SCREEN_W,bottomH,{8,6,3,240});
    DrawLine(0,barY,SCREEN_W,barY,{120,95,45,180});

    // Difficulty
    DrawText("Difficulty:",(int)20,(int)(barY+10),13,{200,185,140,255});
    cfg.difficulty=drawSlider({20,(float)(barY+26),300,16},cfg.difficulty,0.5f,2.0f,nullptr,"%.2fx",mouse);
    const char* dlbl=cfg.difficulty<0.8f?"Easy":cfg.difficulty<1.2f?"Normal":cfg.difficulty<1.6f?"Hard":"BRUTAL";
    Color dcol=cfg.difficulty<0.8f?GREEN:cfg.difficulty<1.2f?YELLOW:cfg.difficulty<1.6f?ORANGE:RED;
    DrawText(dlbl,328,(int)(barY+28),13,dcol);

    int pTotal=0,eTotal=0;
    for(int v:cfg.playerCounts) pTotal+=v;
    for(int v:cfg.enemyCounts) eTotal+=v;
    bool canStart=pTotal>0&&eTotal>0;
    Color sc=canStart?Color{35,25,10,255}:Color{30,25,20,255};
    Color sh=canStart?Color{65,50,20,255}:Color{30,25,20,255};
    if(drawButton({(float)(SCREEN_W-370),(float)(barY+10),240,42},"COMMENCE BATTLE",mouse,sc,sh)&&canStart){
        g_customInit=false;
        return STATE_PLAYING;
    }
    if(!canStart) DrawText("Add units to both sides",(int)(SCREEN_W-370),(int)(barY+55),12,{180,100,50,255});
    if(drawButton({(float)(SCREEN_W-120),(float)(barY+10),108,42},"BACK",mouse,{45,25,12,255},{80,40,20,255})){
        g_customInit=false; return STATE_MENU;
    }

    return STATE_CUSTOM_BATTLE;
}

// ═══════════════════════════════════════════════════════════════
//  UNIT EDITOR
// ═══════════════════════════════════════════════════════════════
static int    g_editTypeIdx=0;
static bool   g_dropdownOpen=false;
static bool   g_editorTexDirty=false;
static bool   g_creatingNew=false;
static UnitTypeDef g_newDraft={};
static int    g_nameEditActive=0;
static Texture2D  g_previewTex={0};
static Texture2D  g_draftTex={0};

void rebuildEditorPreview(){
    if(g_previewTex.id>0) UnloadTexture(g_previewTex);
    if(g_editTypeIdx<unitTypeCount()){
        const UnitTypeDef& td=g_unitTypes[g_editTypeIdx];
        Color ac={td.r,td.g,td.b,255}, hc={td.hr,td.hg,td.hb,255};
        Image img;
        if(td.range>0&&td.role==ROLE_RANGED)
            img=makeRangedSprite(ac,hc,td.isCrossbow);
        else
            img=makeUnitSprite(ac,hc,C_WEAPON_METAL,td.hasShield,td.isPolearm,td.isCav,td.horseCol);
        g_previewTex=imgToTex(img);
    }
}

GameState updateDrawUnitEditor(Vector2 mouse){
    if(g_editTypeIdx>=unitTypeCount()) g_editTypeIdx=0;

    ClearBackground({10,8,5,255});
    for(int x=0;x<SCREEN_W;x+=48) DrawLine(x,0,x,SCREEN_H,{22,18,10,60});
    for(int y=0;y<SCREEN_H;y+=48) DrawLine(0,y,SCREEN_W,y,{22,18,10,60});

    DrawRectangle(0,0,SCREEN_W,54,{6,5,3,240});
    DrawRectangleLinesEx({0,54,(float)SCREEN_W,1},1,{120,95,45,180});
    DrawText("UNIT EDITOR",18,10,28,{220,185,100,255});
    DrawText("Edit unit stats — changes apply immediately in battle",18,42,12,{140,115,60,200});

    int headerH=58, btmH=52, panelY=headerH+4;
    int panelH=SCREEN_H-panelY-btmH;
    int leftW=480, rightX=leftW+18, rightW=SCREEN_W-rightX-8;

    DrawRectangle(8,panelY,leftW,panelH,{14,11,6,210});
    DrawRectangleLinesEx({8,(float)panelY,(float)leftW,(float)panelH},1,{90,70,30,200});
    DrawRectangle(rightX,panelY,rightW,panelH,{10,8,5,210});
    DrawRectangleLinesEx({(float)rightX,(float)panelY,(float)rightW,(float)panelH},1,{90,70,30,200});

    float lx=18, ly=(float)panelY+10;

    // Dropdown
    DrawText("Unit Type:",(int)lx,(int)ly,13,{200,185,140,255}); ly+=18;
    if(g_editTypeIdx<unitTypeCount()){
        const UnitTypeDef& cur=g_unitTypes[g_editTypeIdx];
        Rectangle dropBtn={lx,ly,(float)(leftW-20),28};
        bool dhv=CheckCollisionPointRec(mouse,dropBtn);
        DrawRectangleRec(dropBtn,dhv?Color{35,28,14,255}:Color{22,18,8,255});
        DrawRectangleLinesEx(dropBtn,1,g_dropdownOpen?Color{180,145,70,255}:Color{80,65,30,255});
        Color patch={cur.r,cur.g,cur.b,255};
        DrawRectangle((int)lx+4,(int)ly+6,16,16,patch);
        DrawText(cur.name,(int)(lx+26),(int)(ly+6),15,{220,185,100,255});
        int tx=(int)(lx+leftW-30), ty=(int)(ly+14);
        DrawTriangle({(float)tx,(float)(ty-5)},{(float)(tx-6),(float)(ty+5)},{(float)(tx+6),(float)(ty+5)},
                     g_dropdownOpen?Color{220,185,100,255}:Color{160,130,70,255});
        if(dhv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) g_dropdownOpen=!g_dropdownOpen;

        if(g_dropdownOpen){
            float oy=ly+30;
            for(int i=0;i<unitTypeCount();i++){
                const UnitTypeDef& td=g_unitTypes[i];
                bool sel=(i==g_editTypeIdx);
                Rectangle opt={lx,oy,(float)(leftW-20),26};
                bool ohv=CheckCollisionPointRec(mouse,opt);
                DrawRectangleRec(opt,sel?Color{50,38,18,255}:ohv?Color{32,26,12,255}:Color{16,13,6,240});
                DrawRectangleLinesEx(opt,1,sel?Color{180,145,70,255}:Color{60,50,22,255});
                Color op={td.r,td.g,td.b,255};
                DrawRectangle((int)lx+4,(int)oy+5,14,14,op);
                DrawText(td.name,(int)(lx+24),(int)(oy+5),14,sel?Color{220,185,100,255}:Color{180,160,100,255});
                if(ohv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                    g_editTypeIdx=i; g_dropdownOpen=false; g_editorTexDirty=true;
                }
                oy+=27;
            }
            if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                float listH=27.0f*unitTypeCount();
                if(!CheckCollisionPointRec(mouse,{lx,ly,(float)(leftW-20),28+listH}))
                    g_dropdownOpen=false;
            }
            ly+=(float)(27*unitTypeCount())+36;
        } else ly+=36;
    }

    if(g_editTypeIdx>=unitTypeCount()) goto skipEditorSliders;
    {
        UnitTypeDef& td=g_unitTypes[g_editTypeIdx];
        bool changed=false;
        DrawLine((int)lx,(int)ly,(int)(lx+leftW-20),(int)ly,{80,65,30,120}); ly+=8;
        DrawText("COMBAT STATS",(int)lx,(int)ly,12,{180,150,80,255}); ly+=17;
        float sw=leftW-80;

        auto slid=[&](float& val,float lo,float hi,const char* lbl,const char* fmt,Color fc)->bool{
            float nv=drawSlider({lx,ly,sw,16},val,lo,hi,lbl,fmt,mouse,fc);
            bool ch=(nv!=val); val=nv; ly+=36; return ch;
        };

        changed|=slid(td.healthPerEntity,10,200,"HP per Entity","%.0f",{60,200,60,200});
        changed|=slid(td.armour,0,100,"Armour","%.0f",{180,180,100,200});
        changed|=slid(td.speed,30,300,"Speed","%.0f",{80,160,220,200});
        {float v=(float)td.meleeAttack;
         float nv=drawSlider({lx,ly,sw,16},v,1,60,"Melee Attack","%.0f",mouse,{220,160,60,200});
         if(nv!=v){td.meleeAttack=(int)nv;changed=true;} ly+=36;}
        changed|=slid(td.meleeAttackInterval,0.3f,3.0f,"Melee Interval (s)","%.2f",{200,100,60,200});
        {float v=(float)td.meleeDefense;
         float nv=drawSlider({lx,ly,sw,16},v,0,60,"Melee Defense","%.0f",mouse,{80,200,120,200});
         if(nv!=v){td.meleeDefense=(int)nv;changed=true;} ly+=36;}
        changed|=slid(td.meleeBaseDmg,1,100,"Melee Base Dmg","%.0f",{220,140,60,200});
        changed|=slid(td.meleeAPDmg,0,60,"Melee AP Dmg","%.0f",{200,100,60,200});

        if(td.range>0){
            DrawLine((int)lx,(int)ly,(int)(lx+leftW-20),(int)ly,{80,65,30,120}); ly+=6;
            DrawText("RANGED STATS",(int)lx,(int)ly,12,{140,200,120,255}); ly+=15;
            changed|=slid(td.range,50,400,"Range","%.0f",{140,200,120,200});
            changed|=slid(td.missileBaseDmg,1,80,"Missile Base Dmg","%.0f",{180,220,80,200});
            changed|=slid(td.missileAPDmg,0,50,"Missile AP Dmg","%.0f",{160,200,60,200});
            changed|=slid(td.missileReloadTime,0.5f,6.0f,"Reload Time (s)","%.2f",{200,160,60,200});
        }

        // Color
        DrawLine((int)lx,(int)ly,(int)(lx+leftW-20),(int)ly,{80,65,30,120}); ly+=6;
        DrawText("BODY COLOR",(int)lx,(int)ly,12,{180,150,80,255}); ly+=15;
        float nr=(float)td.r, ng=(float)td.g, nb=(float)td.b;
        nr=drawSlider({lx+16,ly,sw,13},nr,0,255,nullptr,"%.0f",mouse,{180,40,40,200});
        DrawRectangle((int)lx,(int)ly,12,13,{(unsigned char)nr,0,0,255}); ly+=26;
        ng=drawSlider({lx+16,ly,sw,13},ng,0,255,nullptr,"%.0f",mouse,{40,180,40,200});
        DrawRectangle((int)lx,(int)ly,12,13,{0,(unsigned char)ng,0,255}); ly+=26;
        nb=drawSlider({lx+16,ly,sw,13},nb,0,255,nullptr,"%.0f",mouse,{40,40,180,200});
        DrawRectangle((int)lx,(int)ly,12,13,{0,0,(unsigned char)nb,255}); ly+=26;
        if((unsigned char)nr!=td.r||(unsigned char)ng!=td.g||(unsigned char)nb!=td.b){
            td.r=(unsigned char)nr; td.g=(unsigned char)ng; td.b=(unsigned char)nb; changed=true;
        }
        DrawRectangle((int)lx,(int)ly,40,16,{td.r,td.g,td.b,255});
        DrawRectangleLinesEx({(float)lx,(float)ly,40,16},1,WHITE);
        DrawText(TextFormat("RGB(%d,%d,%d)",td.r,td.g,td.b),(int)(lx+46),(int)(ly+2),12,{180,160,100,255});

        if(changed){ rebuildTexture(g_editTypeIdx); g_editorTexDirty=true; }
        if(g_editorTexDirty){ rebuildEditorPreview(); g_editorTexDirty=false; }
    }
    skipEditorSliders:;

    // Right panel: preview
    if(g_editTypeIdx<unitTypeCount()){
        const UnitTypeDef& td=g_unitTypes[g_editTypeIdx];
        float px2=(float)rightX, py=(float)panelY, pw=(float)rightW, ph=(float)panelH;

        DrawText(TextFormat("PREVIEW: %s",td.name),(int)(px2+12),(int)(py+12),16,{220,185,100,255});
        DrawLine((int)px2,(int)(py+34),(int)(px2+pw),(int)(py+34),{80,65,30,120});

        static float pAngle=0; pAngle+=45.0f*GetFrameTime();
        Vector2 center={px2+pw/2, py+ph*0.33f};
        if(g_previewTex.id>0){
            drawSprite(g_previewTex,center,pAngle,10.0f,WHITE);
            DrawCircleLines((int)center.x,(int)center.y,70,{120,95,45,80});
        }

        float sy=(float)(py+ph*0.55f);
        DrawText("CURRENT STATS",(int)(px2+12),(int)sy,13,{180,150,80,255}); sy+=20;

        auto srow=[&](const char* lbl,const char* val,Color vc){
            DrawText(lbl,(int)(px2+12),(int)sy,13,{160,140,90,255});
            DrawText(val,(int)(px2+180),(int)sy,13,vc);
            sy+=18;
        };
        int totalHP=(int)(td.numEntities*td.healthPerEntity);
        srow("Entities:",   TextFormat("%d",td.numEntities),WHITE);
        srow("Total HP:",   TextFormat("%d",totalHP),{60,220,60,255});
        srow("Armour:",     TextFormat("%.0f%%",td.armour),{200,200,100,255});
        srow("Speed:",      TextFormat("%.0f",td.speed),{100,180,220,255});
        srow("Melee Atk:",  TextFormat("%d",td.meleeAttack),{220,160,60,255});
        srow("Melee Def:",  TextFormat("%d",td.meleeDefense),{100,200,120,255});
        float totalMelee=td.meleeBaseDmg+td.meleeAPDmg;
        srow("Total Melee:",TextFormat("%.0f (%.0f+%.0fAP)",totalMelee,td.meleeBaseDmg,td.meleeAPDmg),YELLOW);
        float dps=totalMelee/td.meleeAttackInterval;
        srow("Melee DPS:",  TextFormat("%.1f",dps),ORANGE);
        if(td.range>0){
            srow("Range:",      TextFormat("%.0f",td.range),{140,220,100,255});
            float totalMiss=td.missileBaseDmg+td.missileAPDmg;
            srow("Total Missile:",TextFormat("%.0f (%.0f+%.0fAP)",totalMiss,td.missileBaseDmg,td.missileAPDmg),{220,220,100,255});
            srow("Reload:",     TextFormat("%.2fs",td.missileReloadTime),{180,160,80,255});
        }
        srow("Role:",       roleNames[td.role],{180,200,220,255});
        srow("Cost:",       TextFormat("%d gold",td.cost),{220,185,100,255});

        DrawLine((int)px2,(int)sy,(int)(px2+pw),(int)sy,{80,65,30,80}); sy+=8;
        if(td.isBuiltin) DrawText("Built-in unit",(int)(px2+12),(int)sy,12,{120,100,60,200});
        else             DrawText("Custom unit",(int)(px2+12),(int)sy,12,{100,180,80,200});
    }

    // Create new unit dialog
    static Texture2D draftPreviewTex={0};

    if(g_creatingNew){
        DrawRectangle(0,0,SCREEN_W,SCREEN_H,{0,0,0,200});
        float dw=680, dh=560;
        float dx=(SCREEN_W-dw)/2, dy=(SCREEN_H-dh)/2;
        DrawRectangle((int)dx,(int)dy,(int)dw,(int)dh,{14,11,6,248});
        DrawRectangleLinesEx({dx,dy,dw,dh},2,{180,145,70,255});
        DrawText("CREATE NEW UNIT TYPE",(int)(dx+20),(int)(dy+14),18,{220,185,100,255});
        DrawLine((int)dx,(int)(dy+40),(int)(dx+dw),(int)(dy+40),{150,120,50,100});

        float cx=dx+20, cy=dy+54;

        DrawText("Name:",(int)cx,(int)cy,14,{200,185,140,255}); cy+=18;
        Rectangle nameBox={cx,cy,260,26};
        bool nhv=CheckCollisionPointRec(mouse,nameBox);
        DrawRectangleRec(nameBox,g_nameEditActive?Color{22,18,8,255}:Color{14,11,5,255});
        DrawRectangleLinesEx(nameBox,1,g_nameEditActive?Color{180,145,70,255}:Color{80,65,30,255});
        DrawText(g_newDraft.name,(int)(cx+6),(int)(cy+5),14,{220,185,100,255});
        if(g_nameEditActive) DrawText("|",(int)(cx+6+MeasureText(g_newDraft.name,14)),(int)(cy+4),14,{220,185,100,255});
        if(nhv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) g_nameEditActive=1;
        else if(!nhv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) g_nameEditActive=0;
        if(g_nameEditActive){
            int key=GetCharPressed();
            while(key>0){
                int len=strlen(g_newDraft.name);
                if(key>=32&&len<46){g_newDraft.name[len]=(char)key;g_newDraft.name[len+1]='\0';}
                key=GetCharPressed();
            }
            if(IsKeyPressed(KEY_BACKSPACE)){int len=strlen(g_newDraft.name);if(len>0)g_newDraft.name[len-1]='\0';}
        }
        cy+=34;

        // Role selector
        DrawText("Role:",(int)cx,(int)cy,13,{200,185,140,255}); cy+=17;
        for(int r=0;r<ROLE_COUNT;r++){
            bool sel=(g_newDraft.role==(UnitRole)r);
            float bw=(dw-50.0f)/ROLE_COUNT;
            Rectangle rb={cx+r*bw,cy,bw-4,24};
            bool rhv=CheckCollisionPointRec(mouse,rb);
            DrawRectangleRec(rb,sel?Color{50,38,18,255}:rhv?Color{30,24,10,255}:Color{18,14,6,255});
            DrawRectangleLinesEx(rb,1,sel?Color{180,145,70,255}:Color{70,55,25,255});
            int rnw=MeasureText(roleNames[r],11);
            DrawText(roleNames[r],(int)(rb.x+bw/2-rnw/2),(int)(rb.y+5),11,sel?Color{220,185,100,255}:Color{160,140,80,255});
            if(rhv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                g_newDraft.role=(UnitRole)r;
                g_newDraft.isCav=(r==ROLE_LIGHT_CAV||r==ROLE_HEAVY_CAV);
                g_newDraft.range=(r==ROLE_RANGED)?150.0f:0.0f;
                if(draftPreviewTex.id>0) UnloadTexture(draftPreviewTex);
                Color ac2={g_newDraft.r,g_newDraft.g,g_newDraft.b,255};
                Color hc2={g_newDraft.hr,g_newDraft.hg,g_newDraft.hb,255};
                Image di2 = g_newDraft.range>0&&g_newDraft.role==ROLE_RANGED ?
                    makeRangedSprite(ac2,hc2,g_newDraft.isCrossbow) :
                    makeUnitSprite(ac2,hc2,C_WEAPON_METAL,g_newDraft.hasShield,g_newDraft.isPolearm,g_newDraft.isCav,g_newDraft.horseCol);
                draftPreviewTex=imgToTex(di2);
            }
        }
        cy+=32;

        // Sliders
        float sw2=250;
        auto ds=[&](float& val,float lo,float hi,const char* lbl,const char* fmt,Color fc){
            float nv=drawSlider({cx,cy,sw2,15},val,lo,hi,lbl,fmt,mouse,fc);
            bool ch=(nv!=val); if(ch){val=nv;
                if(draftPreviewTex.id>0) UnloadTexture(draftPreviewTex);
                Color ac2={g_newDraft.r,g_newDraft.g,g_newDraft.b,255};
                Color hc2={g_newDraft.hr,g_newDraft.hg,g_newDraft.hb,255};
                Image di2 = g_newDraft.range>0&&g_newDraft.role==ROLE_RANGED ?
                    makeRangedSprite(ac2,hc2,g_newDraft.isCrossbow) :
                    makeUnitSprite(ac2,hc2,C_WEAPON_METAL,g_newDraft.hasShield,g_newDraft.isPolearm,g_newDraft.isCav,g_newDraft.horseCol);
                draftPreviewTex=imgToTex(di2);}
            cy+=32;
        };
        ds(g_newDraft.healthPerEntity,10,200,"HP/Entity","%.0f",{60,200,60,200});
        ds(g_newDraft.armour,0,100,"Armour","%.0f",{180,180,80,200});
        ds(g_newDraft.speed,30,300,"Speed","%.0f",{80,160,220,200});
        {float v=(float)g_newDraft.meleeAttack;
         float nv=drawSlider({cx,cy,sw2,15},v,1,60,"Melee Attack","%.0f",mouse,{220,160,60,200});
         if(nv!=v) g_newDraft.meleeAttack=(int)nv; cy+=32;}
        ds(g_newDraft.meleeBaseDmg,1,80,"Melee Base Dmg","%.0f",{200,130,50,200});
        ds(g_newDraft.meleeAPDmg,0,50,"Melee AP Dmg","%.0f",{180,100,50,200});
        if(g_newDraft.range>0){
            ds(g_newDraft.range,50,400,"Range","%.0f",{140,200,120,200});
            ds(g_newDraft.missileBaseDmg,1,80,"Missile Base Dmg","%.0f",{180,220,80,200});
            ds(g_newDraft.missileReloadTime,0.5f,6,"Reload Time","%.2f",{200,160,60,200});
        }

        // Draft preview sprite
        static float draftAngle=0; draftAngle+=40*GetFrameTime();
        if(draftPreviewTex.id>0){
            Vector2 dc={dx+dw-140,dy+200};
            drawSprite(draftPreviewTex,dc,draftAngle,9.0f,WHITE);
            DrawCircleLines((int)dc.x,(int)dc.y,55,{120,95,45,80});
        }

        bool nameOk=strlen(g_newDraft.name)>0;
        Color cc=nameOk?Color{25,18,8,255}:Color{25,20,15,255};
        Color ch2=nameOk?Color{55,42,18,255}:Color{25,20,15,255};
        if(drawButton({dx+20,dy+dh-52,200,40},"CREATE UNIT",mouse,cc,ch2)&&nameOk){
            g_newDraft.isBuiltin=false;
            g_newDraft.numEntities=std::max(8,g_newDraft.numEntities);
            g_unitTypes.push_back(g_newDraft);
            int ni=(int)g_unitTypes.size()-1;
            rebuildTexture(ni);
            g_editTypeIdx=ni;
            g_creatingNew=false; g_editorTexDirty=true;
            if(draftPreviewTex.id>0){UnloadTexture(draftPreviewTex);draftPreviewTex={0};}
            rebuildEditorPreview();
        }
        if(drawButton({dx+240,dy+dh-52,140,40},"CANCEL",mouse,{50,22,10,255},{90,38,18,255})){
            g_creatingNew=false;
            if(draftPreviewTex.id>0){UnloadTexture(draftPreviewTex);draftPreviewTex={0};}
        }
    } else {
        int btnY=SCREEN_H-46;
        if(drawButton({8,(float)btnY,180,34},"+ NEW UNIT",mouse,{20,40,60,255},{35,70,110,255})){
            memset(&g_newDraft,0,sizeof(g_newDraft));
            strcpy(g_newDraft.name,"New Unit");
            g_newDraft.numEntities=40; g_newDraft.cost=200;
            g_newDraft.healthPerEntity=50; g_newDraft.armour=10; g_newDraft.speed=100;
            g_newDraft.meleeAttack=20; g_newDraft.meleeAttackInterval=1.2f; g_newDraft.meleeDefense=15;
            g_newDraft.meleeBaseDmg=12; g_newDraft.meleeAPDmg=4;
            g_newDraft.r=180; g_newDraft.g=140; g_newDraft.b=80;
            g_newDraft.hr=160; g_newDraft.hg=130; g_newDraft.hb=70;
            g_newDraft.hasShield=true; g_newDraft.isPolearm=false; g_newDraft.isCav=false;
            g_newDraft.horseCol={120,80,45,255};
            g_creatingNew=true; g_nameEditActive=1;
        }
        if(drawButton({196,(float)btnY,130,34},"BACK",mouse,{45,25,12,255},{80,40,20,255}))
            return STATE_MENU;
    }

    return STATE_UNIT_EDITOR;
}

// ═══════════════════════════════════════════════════════════════
//  GAME INIT
// ═══════════════════════════════════════════════════════════════
static std::vector<BattleUnit> g_units;
static std::vector<Projectile> g_projectiles;

void spawnEntities(BattleUnit& bu){
    const UnitTypeDef& td=g_unitTypes[bu.typeIdx];
    bu.entities.clear();
    int n=td.numEntities;
    // Arrange in a grid formation
    int cols=std::max(1,(int)sqrtf((float)n));
    int rows=(n+cols-1)/cols;
    float spacing=td.isCav?40.0f:22.0f;
    float totalW=(cols-1)*spacing, totalH=(rows-1)*spacing;
    for(int i=0;i<n;i++){
        int c=i%cols, r=i/cols;
        Entity e;
        e.pos={bu.pos.x+(c-cols/2)*spacing-totalW*0.5f+(float)rand()/RAND_MAX*4-2,
               bu.pos.y+(r-rows/2)*spacing-totalH*0.5f+(float)rand()/RAND_MAX*4-2};
        e.hp=td.healthPerEntity;
        e.alive=true;
        bu.entities.push_back(e);
    }
}

void initGame(const BattleConfig& cfg){
    g_units.clear(); g_projectiles.clear();

    // Player units — left side
    float px=80, py=100;
    for(int t=0;t<unitTypeCount();t++){
        int cnt=t<(int)cfg.playerCounts.size()?cfg.playerCounts[t]:0;
        if(cnt==0) continue;
        for(int i=0;i<cnt;i++){
            BattleUnit bu={};
            bu.typeIdx=t; bu.isPlayer=true; bu.selected=false;
            const UnitTypeDef& td=g_unitTypes[t];
            bu.pos={px, py};
            bu.target=bu.pos;
            bu.angle=90; bu.angleTarget=90;
            bu.moving=false; bu.targetUnitIdx=-1; bu.forceAttack=false;
            bu.missileReloadTimer=0; bu.meleeAttackTimer=0;
            spawnEntities(bu);
            g_units.push_back(bu);
            py+=td.isCav?90:60;
            if(py>SCREEN_H-80){ py=100; px+=100; }
        }
    }

    // Enemy units — right side
    float ex=SCREEN_W-100, ey=100;
    for(int t=0;t<unitTypeCount();t++){
        int cnt=t<(int)cfg.enemyCounts.size()?cfg.enemyCounts[t]:0;
        if(cnt==0) continue;
        for(int i=0;i<cnt;i++){
            BattleUnit bu={};
            bu.typeIdx=t; bu.isPlayer=false; bu.selected=false;
            const UnitTypeDef& td=g_unitTypes[t];
            bu.pos={ex, ey};
            bu.target=bu.pos;
            bu.angle=270; bu.angleTarget=270;
            bu.moving=false; bu.targetUnitIdx=-1; bu.forceAttack=false;
            bu.missileReloadTimer=0; bu.meleeAttackTimer=0;

            // Scale enemy stats by difficulty
            // We'll apply difficulty at the entity HP level
            auto& tdm=g_unitTypes[t]; // can't modify, use multiplier on the unit

            spawnEntities(bu);
            // Apply difficulty to entity HP
            for(auto& e:bu.entities) e.hp*=cfg.difficulty;
            bu.entities[0].hp*=1; // reset first one (done above already)
            // Actually set maxHP tracking via entity max
            ey+=td.isCav?90:60;
            if(ey>SCREEN_H-80){ ey=100; ex-=100; }
            g_units.push_back(bu);
        }
    }
}

// ═══════════════════════════════════════════════════════════════
//  HIT CHANCE
// ═══════════════════════════════════════════════════════════════
float calcHitChance(int attMelee, int defMelee){
    float hc = 0.35f + (attMelee - defMelee)*0.01f;
    if(hc < 0.10f) hc=0.10f;
    if(hc > 0.90f) hc=0.90f;
    return hc;
}

// Calc armor reduction for non-AP damage
float calcArmorReduction(float armour){
    float maxRed=armour/100.0f;
    float minRed=maxRed*0.5f;
    return frandRange(minRed,maxRed);
}

// ═══════════════════════════════════════════════════════════════
//  BATTLE UPDATE + DRAW
// ═══════════════════════════════════════════════════════════════
GameState updateDrawGame(const BattleConfig& cfg, Vector2 mouse, float dt){

    static Vector2 selStart={0,0};
    static bool dragging=false;
    static Rectangle selRect={0,0,0,0};

    // UI buttons (always on top)
    bool menuPressed=drawButton({(float)(SCREEN_W-120),6,112,30},"MENU",mouse,
                                {30,22,8,220},{65,50,20,255});
    if(menuPressed||IsKeyPressed(KEY_ESCAPE)) return STATE_MENU;
    bool overMenuBtn=btnHover({(float)(SCREEN_W-120),6,112,30},mouse);

    // ── Selection & orders ──────────────────────────────────
    if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)&&!overMenuBtn){
        selStart=mouse; dragging=true;
        for(auto& u:g_units) if(u.isPlayer) u.selected=false;
    }
    if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)&&dragging){
        selRect.x=fminf(mouse.x,selStart.x);
        selRect.y=fminf(mouse.y,selStart.y);
        selRect.width=fabsf(mouse.x-selStart.x);
        selRect.height=fabsf(mouse.y-selStart.y);
    }
    if(IsMouseButtonReleased(MOUSE_LEFT_BUTTON)){
        dragging=false;
        if(selRect.width<6&&selRect.height<6){
            // Try to select a player unit
            float best=9999; int idx=-1;
            for(int i=0;i<(int)g_units.size();i++){
                if(!g_units[i].isPlayer) continue;
                if(g_units[i].entities.empty()) continue;
                // count alive
                int alive=0; for(auto& e:g_units[i].entities) if(e.alive) alive++;
                if(alive==0) continue;
                float d=vdist(mouse,g_units[i].pos);
                if(d<best&&d<50){best=d;idx=i;}
            }
            if(idx>=0) g_units[idx].selected=true;
        } else {
            for(auto& u:g_units){
                if(!u.isPlayer) continue;
                int alive=0; for(auto& e:u.entities) if(e.alive) alive++;
                if(alive==0) continue;
                if(CheckCollisionPointRec(u.pos,selRect)) u.selected=true;
            }
        }
        selRect={0,0,0,0};
    }

    if(IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)){
        // Check if clicking on an enemy unit → attack order
        int clickedEnemy=-1;
        float bestD=9999;
        for(int i=0;i<(int)g_units.size();i++){
            if(g_units[i].isPlayer) continue;
            int alive=0; for(auto& e:g_units[i].entities) if(e.alive) alive++;
            if(alive==0) continue;
            float d=vdist(mouse,g_units[i].pos);
            if(d<50&&d<bestD){bestD=d;clickedEnemy=i;}
        }

        int orderIdx=0;
        for(auto& u:g_units){
            if(!u.isPlayer||!u.selected) continue;
            if(clickedEnemy>=0){
                u.targetUnitIdx=clickedEnemy;
                u.forceAttack=true;
                u.moving=true;
            } else {
                u.targetUnitIdx=-1; u.forceAttack=false;
                u.target={mouse.x+(orderIdx%3-1)*55.0f, mouse.y+(orderIdx/3)*55.0f};
                u.moving=true;
            }
            orderIdx++;
        }
    }

    // ── UNIT LOGIC ───────────────────────────────────────────
    int numUnits=(int)g_units.size();
    for(int ui=0;ui<numUnits;ui++){
        BattleUnit& u=g_units[ui];
        int aliveCount=0;
        for(auto& e:u.entities) if(e.alive) aliveCount++;
        if(aliveCount==0) continue;

        const UnitTypeDef& td=g_unitTypes[u.typeIdx];

        u.meleeAttackTimer=fmaxf(0,u.meleeAttackTimer-dt);
        u.missileReloadTimer=fmaxf(0,u.missileReloadTimer-dt);

        // Find closest enemy
        int closestEnemy=-1; float closestDist=9999;
        for(int j=0;j<numUnits;j++){
            if(g_units[j].isPlayer==u.isPlayer) continue;
            int ae=0; for(auto& e:g_units[j].entities) if(e.alive) ae++;
            if(ae==0) continue;
            float d=vdist(u.pos,g_units[j].pos);
            if(d<closestDist){closestDist=d;closestEnemy=j;}
        }

        // Auto-target if no forced target
        if(u.targetUnitIdx>=0){
            int ae=0; for(auto& e:g_units[u.targetUnitIdx].entities) if(e.alive) ae++;
            if(ae==0){u.targetUnitIdx=-1; u.forceAttack=false;}
        }
        if(u.targetUnitIdx<0&&closestEnemy>=0) u.targetUnitIdx=closestEnemy;

        bool hasTarget=(u.targetUnitIdx>=0);
        float distToTarget=hasTarget?vdist(u.pos,g_units[u.targetUnitIdx].pos):9999;

        // Determine engagement range
        float engageRange = td.range>0 ? td.range : td.isCav ? 70.0f : 45.0f;
        float meleeRange  = td.isCav   ? 70.0f : 45.0f;

        // Move toward target or position
        Vector2 moveTarget=u.target;
        if(hasTarget && (u.forceAttack||distToTarget<engageRange+200)){
            moveTarget=g_units[u.targetUnitIdx].pos;
        }

        float distToMove=vdist(u.pos,moveTarget);
        bool shouldMove = u.moving && distToMove > (hasTarget&&td.range>0 ? engageRange*0.8f : 12.0f);
        // Stop approaching melee target when in range
        if(hasTarget && td.range==0 && distToTarget<meleeRange) shouldMove=false;
        if(hasTarget && td.range>0  && distToTarget<engageRange*0.9f) shouldMove=false;

        if(shouldMove){
            Vector2 dir=vnorm({moveTarget.x-u.pos.x,moveTarget.y-u.pos.y});
            float moveSpeed=td.speed;
            u.pos.x+=dir.x*moveSpeed*dt;
            u.pos.y+=dir.y*moveSpeed*dt;
            u.angleTarget=dirToAngle(dir);
            // Move entities with unit
            for(auto& e:u.entities){
                e.pos.x+=dir.x*moveSpeed*dt;
                e.pos.y+=dir.y*moveSpeed*dt;
            }
        } else if(!shouldMove && distToMove<12) {
            u.moving=false;
        }

        if(hasTarget){
            Vector2 dir=vnorm({g_units[u.targetUnitIdx].pos.x-u.pos.x,
                               g_units[u.targetUnitIdx].pos.y-u.pos.y});
            u.angleTarget=dirToAngle(dir);
        }

        u.angle=lerpAngle(u.angle,u.angleTarget,8.0f*dt);

        // RANGED ATTACK
        if(hasTarget && td.range>0 && distToTarget<=td.range && u.missileReloadTimer<=0){
            // Find alive enemy entity
            BattleUnit& tgtUnit=g_units[u.targetUnitIdx];
            Entity* tgtEnt=nullptr;
            for(auto& e:tgtUnit.entities) if(e.alive){tgtEnt=&e;break;}
            if(tgtEnt){
                Vector2 dir=vnorm({tgtEnt->pos.x-u.pos.x,tgtEnt->pos.y-u.pos.y});
                // Shoot from a random entity
                int shooterIdx=-1;
                for(int k=0;k<(int)u.entities.size();k++) if(u.entities[k].alive){shooterIdx=k;break;}
                if(shooterIdx>=0){
                    Projectile proj;
                    proj.pos=u.entities[shooterIdx].pos;
                    proj.vel={dir.x*300,dir.y*300};
                    proj.damage=td.missileBaseDmg;
                    proj.apDamage=td.missileAPDmg;
                    proj.alive=true; proj.fromPlayer=u.isPlayer;
                    proj.targetUnitIdx=u.targetUnitIdx;
                    g_projectiles.push_back(proj);
                }
                u.missileReloadTimer=td.missileReloadTime;
            }
        }

        // MELEE ATTACK
        if(hasTarget && distToTarget<=meleeRange+20 && u.meleeAttackTimer<=0){
            const UnitTypeDef& atd=td;
            BattleUnit& tgtUnit=g_units[u.targetUnitIdx];
            const UnitTypeDef& dtd=g_unitTypes[tgtUnit.typeIdx];
            float hitChance=calcHitChance(atd.meleeAttack,dtd.meleeDefense);
            // Each alive attacker attacks a random alive defender
            for(auto& attEnt:u.entities){
                if(!attEnt.alive) continue;
                if((float)rand()/RAND_MAX > hitChance) continue;
                // Pick random alive defender entity
                std::vector<Entity*> defenders;
                for(auto& de:tgtUnit.entities) if(de.alive) defenders.push_back(&de);
                if(defenders.empty()) break;
                Entity* defEnt=defenders[rand()%defenders.size()];
                // Apply damage
                float armor=calcArmorReduction(dtd.armour);
                float baseDmg=atd.meleeBaseDmg*(1.0f-armor);
                float apDmg=atd.meleeAPDmg;
                defEnt->hp-=(baseDmg+apDmg);
                if(defEnt->hp<=0) defEnt->alive=false;
            }
            u.meleeAttackTimer=atd.meleeAttackInterval;
            // Update tgt unit position centroid
            float sx=0,sy2=0; int ac=0;
            for(auto& e:tgtUnit.entities) if(e.alive){sx+=e.pos.x;sy2+=e.pos.y;ac++;}
            if(ac>0){tgtUnit.pos={sx/ac,sy2/ac};}
        }

        // Update unit centroid position
        float sx=0,sy2=0; int ac=0;
        for(auto& e:u.entities) if(e.alive){sx+=e.pos.x;sy2+=e.pos.y;ac++;}
        if(ac>0) u.pos={sx/ac,sy2/ac};
    }

    // ── PROJECTILE LOGIC ────────────────────────────────────
    for(auto& proj:g_projectiles){
        if(!proj.alive) continue;
        proj.pos.x+=proj.vel.x*dt;
        proj.pos.y+=proj.vel.y*dt;
        if(proj.pos.x<0||proj.pos.x>SCREEN_W||proj.pos.y<0||proj.pos.y>SCREEN_H){proj.alive=false;continue;}

        // Check hit against target unit
        if(proj.targetUnitIdx>=0&&proj.targetUnitIdx<(int)g_units.size()){
            BattleUnit& tgt=g_units[proj.targetUnitIdx];
            for(auto& e:tgt.entities){
                if(!e.alive) continue;
                if(vdist(proj.pos,e.pos)<12){
                    proj.alive=false;
                    // Missile hit
                    const UnitTypeDef& dtd=g_unitTypes[tgt.typeIdx];
                    float armor=calcArmorReduction(dtd.armour);
                    e.hp-=proj.damage*(1.0f-armor)+proj.apDamage;
                    if(e.hp<=0) e.alive=false;
                    break;
                }
            }
        } else {
            // No specific target, hit any enemy
            for(auto& u:g_units){
                if(u.isPlayer==proj.fromPlayer) continue;
                for(auto& e:u.entities){
                    if(!e.alive) continue;
                    if(vdist(proj.pos,e.pos)<10){
                        proj.alive=false;
                        const UnitTypeDef& dtd=g_unitTypes[u.typeIdx];
                        float armor=calcArmorReduction(dtd.armour);
                        e.hp-=proj.damage*(1.0f-armor)+proj.apDamage;
                        if(e.hp<=0) e.alive=false;
                        goto nextProj;
                    }
                }
            }
            nextProj:;
        }
    }

    // Count sides
    int alivePlayer=0, aliveEnemy=0;
    for(auto& u:g_units){
        int ac=0; for(auto& e:u.entities) if(e.alive) ac++;
        if(ac>0){
            if(u.isPlayer) alivePlayer++;
            else aliveEnemy++;
        }
    }

    // ═══════════════ DRAW ═════════════════════════════════
    // Ground
    ClearBackground({30,42,22,255});

    // Terrain grid
    for(int x=0;x<SCREEN_W;x+=80) DrawLine(x,0,x,SCREEN_H,{38,52,28,255});
    for(int y=0;y<SCREEN_H;y+=80) DrawLine(0,y,SCREEN_W,y,{38,52,28,255});

    // Random grass patches
    srand(42);
    for(int i=0;i<60;i++){
        int gx=rand()%SCREEN_W, gy=rand()%SCREEN_H;
        DrawRectangle(gx,gy,4+rand()%8,2+rand()%4,{(unsigned char)(28+rand()%14),(unsigned char)(48+rand()%14),(unsigned char)(18+rand()%8),180});
    }
    srand((unsigned)time(nullptr)); // reset random

    // Draw projectiles
    for(auto& proj:g_projectiles){
        if(!proj.alive) continue;
        Color pc=proj.fromPlayer?Color{220,200,100,255}:Color{220,100,80,255};
        DrawCircleV(proj.pos,3,pc);
        DrawCircleV(proj.pos,2,WHITE);
    }

    // Draw units
    for(int ui=0;ui<(int)g_units.size();ui++){
        BattleUnit& u=g_units[ui];
        const UnitTypeDef& td=g_unitTypes[u.typeIdx];
        Texture2D& tex=u.isPlayer?g_playerTex[u.typeIdx]:g_enemyTex[u.typeIdx];

        int aliveCount=0;
        float totalMaxHp=td.healthPerEntity*(float)td.numEntities;
        float totalHp=0;
        for(auto& e:u.entities){if(e.alive){aliveCount++;totalHp+=e.hp;}}
        if(aliveCount==0) continue;

        // Draw selection ring
        if(u.selected&&u.isPlayer){
            DrawCircleLines((int)u.pos.x,(int)u.pos.y,td.isCav?55:38,{220,200,80,200});
        }

        // Draw target line for selected
        if(u.selected&&u.isPlayer&&(u.moving||u.targetUnitIdx>=0)){
            Vector2 tgt=u.targetUnitIdx>=0?g_units[u.targetUnitIdx].pos:u.target;
            DrawLineBezier(u.pos,tgt,2.0f,{180,160,60,120});
            DrawCircleLines((int)tgt.x,(int)tgt.y,8,{180,160,60,180});
        }

        // Draw individual entities
        float scale=td.isCav?2.0f:1.6f;
        for(auto& e:u.entities){
            if(!e.alive) continue;
            drawSprite(tex,e.pos,u.angle,scale,WHITE);
        }

        // Formation outline (bounding box)
        if(u.selected&&u.isPlayer)
            DrawRectangleLinesEx({u.pos.x-30,u.pos.y-20,60,40},1,{180,160,60,100});

        // Health bar above centroid
        drawHealthBar(u.pos,totalHp,totalMaxHp*(float)aliveCount/(float)td.numEntities+1,40);

        // Entity count
        DrawText(TextFormat("%d/%d",aliveCount,td.numEntities),
                 (int)(u.pos.x-12),(int)(u.pos.y-32),11,u.isPlayer?Color{140,200,255,220}:Color{255,160,140,220});
    }

    // Selection box
    if(dragging&&(selRect.width>6||selRect.height>6)){
        DrawRectangleRec(selRect,{180,160,60,30});
        DrawRectangleLinesEx(selRect,1,{220,200,100,200});
    }

    // ── HUD ────────────────────────────────────────────────
    DrawRectangle(0,0,SCREEN_W,44,{5,4,2,230});
    DrawRectangleLinesEx({0,44,(float)SCREEN_W,1},1,{120,95,45,150});

    // Count total entities
    int playerEntities=0, enemyEntities=0;
    for(auto& u:g_units){
        int ac=0; for(auto& e:u.entities) if(e.alive) ac++;
        if(u.isPlayer) playerEntities+=ac; else enemyEntities+=ac;
    }

    DrawText(TextFormat("YOUR FORCES: %d units (%d men)",alivePlayer,playerEntities),12,14,15,{140,200,140,255});
    DrawText(TextFormat("ENEMY FORCES: %d units (%d men)",aliveEnemy,enemyEntities),
             SCREEN_W/2-100,14,15,{200,140,140,255});
    drawButton({(float)(SCREEN_W-120),6,112,30},"MENU",mouse,{30,22,8,220},{65,50,20,255});

    // ── VICTORY / DEFEAT ───────────────────────────────────
    if(alivePlayer==0&&aliveEnemy==0){
        DrawRectangle(0,0,SCREEN_W,SCREEN_H,{0,0,0,160});
        const char* msg="BATTLE ENDED";
        DrawText(msg,SCREEN_W/2-MeasureText(msg,52)/2,SCREEN_H/2-40,52,{180,160,80,255});
        if(drawButton({SCREEN_W/2-130.0f,(float)(SCREEN_H/2+30),260,50},"RETURN TO MENU",mouse))
            return STATE_MENU;
    } else if(alivePlayer==0){
        DrawRectangle(0,0,SCREEN_W,SCREEN_H,{0,0,0,160});
        const char* msg="DEFEAT";
        DrawText(msg,SCREEN_W/2-MeasureText(msg,60)/2+2,SCREEN_H/2-40+2,60,{120,0,0,255});
        DrawText(msg,SCREEN_W/2-MeasureText(msg,60)/2,  SCREEN_H/2-40,  60,{220,50,50,255});
        if(drawButton({SCREEN_W/2-130.0f,(float)(SCREEN_H/2+30),260,50},"RETURN TO MENU",mouse))
            return STATE_MENU;
    } else if(aliveEnemy==0){
        DrawRectangle(0,0,SCREEN_W,SCREEN_H,{0,0,0,120});
        const char* msg="VICTORY!";
        DrawText(msg,SCREEN_W/2-MeasureText(msg,60)/2+2,SCREEN_H/2-40+2,60,{0,100,0,255});
        DrawText(msg,SCREEN_W/2-MeasureText(msg,60)/2,  SCREEN_H/2-40,  60,{80,220,80,255});
        if(drawButton({SCREEN_W/2-130.0f,(float)(SCREEN_H/2+30),260,50},"RETURN TO MENU",mouse))
            return STATE_MENU;
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
    rebuildEditorPreview();

    GameState    state=STATE_MENU;
    BattleConfig cfg  =defaultBattleConfig();
    float menuTime=0;

    while(!WindowShouldClose()){
        if(IsKeyPressed(KEY_F11)||(IsKeyDown(KEY_LEFT_ALT)&&IsKeyPressed(KEY_ENTER)))
            ToggleFullscreen();
        SCREEN_W=GetScreenWidth();
        SCREEN_H=GetScreenHeight();

        float dt=fminf(GetFrameTime(),0.05f);
        menuTime+=dt;
        Vector2 mouse=GetMousePosition();
        BeginDrawing();

        switch(state){
            case STATE_MENU:{
                GameState nxt=updateDrawMenu(mouse,menuTime);
                // Handle EXIT button manually
                float bx=SCREEN_W/2-170.0f;
                if(btnHover({bx,400,340,56},mouse)&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                    goto quit;
                if(nxt!=STATE_MENU) state=nxt;
                break;
            }
            case STATE_CUSTOM_BATTLE:{
                GameState nxt=updateDrawCustomBattle(cfg,mouse);
                if(nxt==STATE_PLAYING){
                    resizeBattleConfig(cfg);
                    initGame(cfg);
                    state=STATE_PLAYING;
                } else state=nxt;
                break;
            }
            case STATE_UNIT_EDITOR:{
                state=updateDrawUnitEditor(mouse);
                break;
            }
            case STATE_PLAYING:{
                state=updateDrawGame(cfg,mouse,dt);
                if(state==STATE_MENU){
                    g_units.clear(); g_projectiles.clear();
                }
                break;
            }
        }
        EndDrawing();
    }
    quit:
    for(auto& t:g_playerTex) UnloadTexture(t);
    for(auto& t:g_enemyTex)  UnloadTexture(t);
    if(g_previewTex.id>0) UnloadTexture(g_previewTex);
    CloseWindow();
    return 0;
}
