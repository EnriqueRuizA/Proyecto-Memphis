#include "raylib.h"
#include <vector>
#include <cmath>
#include <string>
#include <algorithm>
#include <cstring>

// ═══════════════════════════════════════════════════════════════
//  PANTALLA (dinámica para fullscreen)
// ═══════════════════════════════════════════════════════════════
int SCREEN_W = 1280;
int SCREEN_H = 768;
const int SPR_SIZE = 16;

// ═══════════════════════════════════════════════════════════════
//  SPRITES PIXEL ART (forward declarations de funciones de textura)
// ═══════════════════════════════════════════════════════════════
unsigned char clampU8(int v){ return (unsigned char)std::max(0,std::min(255,v)); }

static void px(Image& img,int x,int y,Color c){
    if(x<0||y<0||x>=img.width||y>=img.height) return;
    ImageDrawPixel(&img,x,y,c);
}

Image makeTankImg(unsigned char r,unsigned char g,unsigned char b){
    Image img=GenImageColor(SPR_SIZE,SPR_SIZE,BLANK);
    Color body={r,g,b,255},dark={clampU8(r/2),clampU8(g/2),clampU8(b/2),255};
    Color hi={clampU8(r+80),clampU8(g+80),clampU8(b+80),255};
    Color bar={200,200,210,255},trk={20,20,20,255};
    for(int y2=4;y2<=13;y2++){px(img,1,y2,trk);px(img,2,y2,trk);px(img,13,y2,trk);px(img,14,y2,trk);}
    for(int y2=5;y2<=12;y2++) for(int x2=3;x2<=12;x2++) px(img,x2,y2,body);
    for(int y2=6;y2<=11;y2++) for(int x2=5;x2<=10;x2++) px(img,x2,y2,dark);
    px(img,5,6,hi);px(img,6,6,hi);px(img,5,7,hi);
    for(int y2=0;y2<=5;y2++){px(img,7,y2,bar);px(img,8,y2,bar);}
    return img;
}
Image makeSoldierImg(unsigned char r,unsigned char g,unsigned char b){
    Image img=GenImageColor(SPR_SIZE,SPR_SIZE,BLANK);
    Color body={r,g,b,255},dark={clampU8(r/2),clampU8(g/2),clampU8(b/2),255};
    Color hi={clampU8(r+80),clampU8(g+80),clampU8(b+80),255};
    Color skin={220,170,120,255},blt={40,40,40,255};
    for(int y2=11;y2<=15;y2++){px(img,5,y2,dark);px(img,6,y2,dark);px(img,9,y2,dark);px(img,10,y2,dark);}
    for(int y2=6;y2<=11;y2++) for(int x2=5;x2<=10;x2++) px(img,x2,y2,body);
    px(img,5,6,hi);px(img,6,6,hi);
    for(int y2=2;y2<=5;y2++) for(int x2=6;x2<=9;x2++) px(img,x2,y2,skin);
    for(int x2=5;x2<=10;x2++) px(img,x2,2,dark);
    px(img,5,3,dark);px(img,10,3,dark);
    px(img,8,0,blt);px(img,8,1,blt);px(img,8,2,blt);px(img,9,2,blt);
    return img;
}
Image makeSniperImg(unsigned char r,unsigned char g,unsigned char b){
    Image img=GenImageColor(SPR_SIZE,SPR_SIZE,BLANK);
    Color body={r,g,b,255},dark={clampU8(r/2),clampU8(g/2),clampU8(b/2),255};
    Color hi={clampU8(r+80),clampU8(g+80),clampU8(b+80),255};
    Color bar={210,210,180,255},scp={80,80,50,255};
    for(int y2=7;y2<=13;y2++) for(int x2=5;x2<=10;x2++) px(img,x2,y2,body);
    for(int y2=5;y2<=8;y2++)  for(int x2=6;x2<=9;x2++)  px(img,x2,y2,dark);
    px(img,6,5,hi);
    for(int y2=0;y2<=5;y2++){px(img,7,y2,bar);px(img,8,y2,bar);}
    px(img,6,2,scp);px(img,7,2,scp);px(img,8,2,scp);px(img,9,2,scp);
    px(img,6,13,dark);px(img,7,14,dark);px(img,8,14,dark);px(img,9,13,dark);
    return img;
}

Texture2D imgToTex(Image img){
    Texture2D tex=LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(tex,TEXTURE_FILTER_POINT);
    return tex;
}

// ═══════════════════════════════════════════════════════════════
//  TIPOS DE UNIDAD — sistema dinámico
// ═══════════════════════════════════════════════════════════════
enum SpriteBase { SPR_TANK=0, SPR_SOLDIER, SPR_SNIPER, SPR_BASE_COUNT };
const char* spriteBaseNames[SPR_BASE_COUNT] = { "Tanque", "Soldado", "Francotirador" };

struct UnitTypeDef {
    char  name[32];
    float speed, hp, damage, attackRange, attackCooldown;
    unsigned char r, g, b;
    SpriteBase spriteBase;
    bool  isBuiltin;
};

static std::vector<UnitTypeDef> g_unitTypes;
static std::vector<Texture2D>   g_playerTextures;  // una textura por tipo
static std::vector<Texture2D>   g_enemyTextures;   // versión rojiza por tipo

void rebuildTexture(int idx){
    const UnitTypeDef& td=g_unitTypes[idx];
    // Player tex
    if(idx<(int)g_playerTextures.size()) UnloadTexture(g_playerTextures[idx]);
    Image img;
    switch(td.spriteBase){
        case SPR_SOLDIER: img=makeSoldierImg(td.r,td.g,td.b); break;
        case SPR_SNIPER:  img=makeSniperImg(td.r,td.g,td.b); break;
        default:          img=makeTankImg(td.r,td.g,td.b); break;
    }
    if(idx<(int)g_playerTextures.size()) g_playerTextures[idx]=imgToTex(img);
    else g_playerTextures.push_back(imgToTex(img));
    // Enemy tex (rojiza)
    unsigned char er=clampU8((int)(td.r*0.5f+180*0.5f));
    unsigned char eg=clampU8((int)(td.g*0.3f));
    unsigned char eb=clampU8((int)(td.b*0.3f));
    Image eimg;
    switch(td.spriteBase){
        case SPR_SOLDIER: eimg=makeSoldierImg(er,eg,eb); break;
        case SPR_SNIPER:  eimg=makeSniperImg(er,eg,eb); break;
        default:          eimg=makeTankImg(er,eg,eb); break;
    }
    if(idx<(int)g_enemyTextures.size()) g_enemyTextures[idx]=imgToTex(eimg);
    else g_enemyTextures.push_back(imgToTex(eimg));
}

void initBuiltinTypes(){
    g_unitTypes.clear();
    UnitTypeDef t;
    strcpy(t.name,"Tanque");
    t.speed=100; t.hp=200; t.damage=30; t.attackRange=90; t.attackCooldown=1.2f;
    t.r=60; t.g=120; t.b=220; t.spriteBase=SPR_TANK; t.isBuiltin=true;
    g_unitTypes.push_back(t);
    strcpy(t.name,"Soldado");
    t.speed=180; t.hp=80; t.damage=18; t.attackRange=70; t.attackCooldown=0.6f;
    t.r=60; t.g=180; t.b=60; t.spriteBase=SPR_SOLDIER; t.isBuiltin=true;
    g_unitTypes.push_back(t);
    strcpy(t.name,"Francotirador");
    t.speed=60; t.hp=100; t.damage=80; t.attackRange=220; t.attackCooldown=2.5f;
    t.r=180; t.g=140; t.b=50; t.spriteBase=SPR_SNIPER; t.isBuiltin=true;
    g_unitTypes.push_back(t);
    // Build textures for all
    g_playerTextures.clear();
    g_enemyTextures.clear();
    for(int i=0;i<(int)g_unitTypes.size();i++) rebuildTexture(i);
}

int unitTypeCount(){ return (int)g_unitTypes.size(); }

// ═══════════════════════════════════════════════════════════════
//  ESTADOS
// ═══════════════════════════════════════════════════════════════
enum GameState { STATE_MENU, STATE_CUSTOM_BATTLE, STATE_UNIT_EDITOR, STATE_PLAYING };

// ═══════════════════════════════════════════════════════════════
//  CONFIGURACIÓN DE BATALLA
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
    if(n>0) c.playerCounts[0]=9;
    if(n>0) c.enemyCounts[0]=6;
    c.difficulty=1.0f;
    return c;
}

void resizeBattleConfig(BattleConfig& c){
    int n=unitTypeCount();
    c.playerCounts.resize(n,0);
    c.enemyCounts.resize(n,0);
}

// ═══════════════════════════════════════════════════════════════
//  ESTRUCTURAS EN PARTIDA
// ═══════════════════════════════════════════════════════════════
struct Unit {
    Vector2  pos, target;
    float    hp, maxHp, speed, damage, attackRange, attackCooldown;
    bool     selected, moving, alive;
    float    angle, angleTarget, attackTimer;
    Color    bodyColor;
    int      typeIdx;
};

struct Enemy {
    Vector2  pos;
    float    hp, maxHp, speed, attackTimer, angle;
    bool     alive;
    int      typeIdx;
};

struct Bullet {
    Vector2 pos, vel;
    float   damage;
    bool    alive, fromPlayer;
};

// ═══════════════════════════════════════════════════════════════
//  MATH / UTILS
// ═══════════════════════════════════════════════════════════════
float vdist(Vector2 a,Vector2 b){float dx=a.x-b.x,dy=a.y-b.y;return sqrtf(dx*dx+dy*dy);}
Vector2 vnorm(Vector2 v){float l=sqrtf(v.x*v.x+v.y*v.y);if(l==0)return{0,0};return{v.x/l,v.y/l};}
float dirToAngle(Vector2 d){return atan2f(-d.y,d.x)*RAD2DEG;}
float lerpAngle(float c,float t,float s){float d=t-c;while(d>180)d-=360;while(d<-180)d+=360;return c+d*s;}
bool  btnHover(Rectangle r,Vector2 m){return CheckCollisionPointRec(m,r);}

int totalUnits(const std::vector<int>& counts){
    int s=0; for(int v:counts) s+=v; return s;
}

// ═══════════════════════════════════════════════════════════════
//  UI HELPERS
// ═══════════════════════════════════════════════════════════════
void drawHealthBar(Vector2 pos,float hp,float maxHp,float w){
    float r=hp/maxHp;
    DrawRectangle((int)(pos.x-w/2),(int)(pos.y-22),(int)w,5,DARKGRAY);
    DrawRectangle((int)(pos.x-w/2),(int)(pos.y-22),(int)(w*r),5,r>0.5f?GREEN:r>0.25f?YELLOW:RED);
}

bool drawButton(Rectangle r,const char* lbl,Vector2 m,
                Color cn={40,60,40,255},Color ch={70,120,60,255}){
    bool hv=btnHover(r,m);
    DrawRectangleRec(r,hv?ch:cn);
    DrawRectangleLinesEx(r,2,hv?GREEN:Color{80,130,80,255});
    int tw=MeasureText(lbl,18);
    DrawText(lbl,(int)(r.x+r.width/2-tw/2),(int)(r.y+r.height/2-9),18,hv?WHITE:LIGHTGRAY);
    return hv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

float drawSlider(Rectangle r,float val,float vmin,float vmax,
                 const char* label,const char* fmt,Vector2 mouse,
                 Color fill={60,180,60,200}){
    if(label&&label[0]) DrawText(label,(int)r.x,(int)(r.y-18),14,LIGHTGRAY);
    DrawRectangleRec(r,{25,25,25,255});
    DrawRectangleLinesEx(r,1,{70,70,70,255});
    float t=(val-vmin)/(vmax-vmin);
    DrawRectangle((int)r.x,(int)r.y,(int)(t*r.width),(int)r.height,fill);
    float kx=r.x+t*r.width;
    DrawRectangle((int)(kx-5),(int)(r.y-3),10,(int)(r.height+6),WHITE);
    DrawText(TextFormat(fmt,val),(int)(r.x+r.width+8),(int)(r.y+1),14,WHITE);
    if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)&&CheckCollisionPointRec(mouse,r)){
        t=(mouse.x-r.x)/r.width; t=fmaxf(0,fminf(1,t));
        val=vmin+t*(vmax-vmin);
    }
    return val;
}

// Dibuja una X como cruz usando líneas (no texto, para evitar problemas de fuente)
void drawXCross(int cx,int cy,int size,Color col){
    int h=size/2;
    DrawLine(cx-h,cy-h,cx+h,cy+h,col);
    DrawLine(cx+h,cy-h,cx-h,cy+h,col);
    // Segunda pasada más gruesa
    DrawLine(cx-h+1,cy-h,cx+h+1,cy+h,col);
    DrawLine(cx+h+1,cy-h,cx-h+1,cy+h,col);
}

void drawSprite(Texture2D tex,Vector2 pos,float angleDeg,float scale,Color tint){
    float s=SPR_SIZE*scale;
    DrawTexturePro(tex,{0,0,(float)SPR_SIZE,(float)SPR_SIZE},
                   {pos.x,pos.y,s,s},{s/2,s/2},-angleDeg+90.0f,tint);
}

// ═══════════════════════════════════════════════════════════════
//  MENÚ PRINCIPAL
// ═══════════════════════════════════════════════════════════════
GameState updateDrawMenu(Vector2 mouse,float t){
    ClearBackground({10,15,10,255});
    for(int x=0;x<SCREEN_W;x+=64)
        DrawLine(x,0,x,SCREEN_H,{20,35,20,(unsigned char)(80+25*(int)sinf(t+x*0.01f))});
    for(int y=0;y<SCREEN_H;y+=64)
        DrawLine(0,y,SCREEN_W,y,{20,35,20,(unsigned char)(80+25*(int)sinf(t+y*0.01f))});

    const char* title="RTS PIXEL WAR";
    int tw=MeasureText(title,60);
    DrawText(title,SCREEN_W/2-tw/2+2,122,60,{0,80,0,200});
    DrawText(title,SCREEN_W/2-tw/2,  120,60,GREEN);
    const char* sub="Estrategia en tiempo real — pixel art";
    DrawText(sub,SCREEN_W/2-MeasureText(sub,18)/2,196,18,{120,180,120,255});

    float bx=SCREEN_W/2-155.0f,bw=310,bh=52;
    if(drawButton({bx,270,bw,bh},"QUICK BATTLE",mouse))        return STATE_PLAYING;
    if(drawButton({bx,338,bw,bh},"CUSTOM BATTLE",mouse))       return STATE_CUSTOM_BATTLE;
    if(drawButton({bx,406,bw,bh},"EDITOR DE UNIDADES",mouse))  return STATE_UNIT_EDITOR;
    drawButton({bx,474,bw,bh},"SALIR",mouse,{60,30,30,255},{110,50,50,255});

    DrawText("Clic IZQ: seleccionar  |  Clic DER: mover  |  Arrastrar: seleccion multiple  |  F11: pantalla completa",
             SCREEN_W/2-MeasureText("Clic IZQ: seleccionar  |  Clic DER: mover  |  Arrastrar: seleccion multiple  |  F11: pantalla completa",12)/2,
             742,12,{70,110,70,255});
    return STATE_MENU;
}

// ═══════════════════════════════════════════════════════════════
//  CUSTOM BATTLE
// ═══════════════════════════════════════════════════════════════
struct UnitEntry { int typeIdx; };

static std::vector<UnitEntry> g_playerUnits;
static std::vector<UnitEntry> g_enemyUnits;
static bool g_customInit=false;

static void syncCounts(BattleConfig& cfg){
    resizeBattleConfig(cfg);
    for(int i=0;i<unitTypeCount();i++){ cfg.playerCounts[i]=0; cfg.enemyCounts[i]=0; }
    for(auto& u:g_playerUnits) if(u.typeIdx<unitTypeCount()) cfg.playerCounts[u.typeIdx]++;
    for(auto& u:g_enemyUnits)  if(u.typeIdx<unitTypeCount()) cfg.enemyCounts[u.typeIdx]++;
}

// Dibuja la lista de un ejército con botón X dibujado (cruz) por unidad
static int drawArmyList(std::vector<UnitEntry>& list,float x,float y,float w,float h,
                         Color borderCol,Color titleCol,const char* title,Vector2 mouse,bool isPlayer){
    DrawRectangle((int)x,(int)y,(int)w,(int)h,{10,15,10,230});
    DrawRectangleLinesEx({x,y,w,h},2,borderCol);
    DrawText(title,(int)(x+12),(int)(y+10),16,titleCol);
    int cnt=(int)list.size();
    DrawText(TextFormat("(%d unidades)",cnt),(int)(x+w-MeasureText(TextFormat("(%d unidades)",cnt),13)-12),(int)(y+13),13,{150,150,150,255});
    DrawLine((int)x,(int)(y+34),(int)(x+w),(int)(y+34),{borderCol.r,borderCol.g,borderCol.b,80});

    float iy=y+40, rowH=30, maxH=h-46;
    int typeCount[64]={};
    int removeIdx=-1;

    for(int i=0;i<cnt;i++){
        if(iy-y>maxH) break;
        int ti=list[i].typeIdx;
        if(ti>=unitTypeCount()) continue;
        int instNum=++typeCount[ti];
        Color rowBg=isPlayer?
            (i%2==0?Color{10,20,40,200}:Color{8,16,32,200}):
            (i%2==0?Color{35,10,10,200}:Color{28,8,8,200});
        DrawRectangle((int)x,(int)iy,(int)w,(int)rowH,rowBg);

        const UnitTypeDef& td=g_unitTypes[ti];
        Color typeCol=isPlayer?
            Color{td.r,td.g,td.b,255}:
            Color{clampU8((int)(td.r*0.5f+90)),clampU8((int)(td.g*0.3f)),clampU8((int)(td.b*0.3f)),255};
        DrawRectangle((int)(x+8),(int)(iy+7),12,16,typeCol);
        DrawText(TextFormat("%s %d",td.name,instNum),(int)(x+26),(int)(iy+8),14,WHITE);

        // Botón X — dibujado como cruz con líneas (no texto)
        float bxb=x+w-36, byb=iy+4;
        bool hv=CheckCollisionPointRec(mouse,{bxb,byb,28,22});
        DrawRectangle((int)bxb,(int)byb,28,22,hv?Color{180,30,30,255}:Color{90,20,20,200});
        DrawRectangleLinesEx({bxb,byb,28,22},1,hv?RED:Color{130,40,40,255});
        Color xCol=hv?WHITE:Color{220,140,140,255};
        int cxb=(int)(bxb+14), cyb=(int)(byb+11);
        DrawLine(cxb-5,cyb-5,cxb+5,cyb+5,xCol);
        DrawLine(cxb+5,cyb-5,cxb-5,cyb+5,xCol);
        DrawLine(cxb-4,cyb-5,cxb+6,cyb+5,xCol);
        DrawLine(cxb+6,cyb-5,cxb-4,cyb+5,xCol);
        if(hv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) removeIdx=i;

        iy+=rowH;
    }
    if(list.empty())
        DrawText("Sin unidades",(int)(x+12),(int)(y+50),14,{80,80,80,255});
    return removeIdx;
}

GameState updateDrawCustomBattle(BattleConfig& cfg, Vector2 mouse){
    if(!g_customInit){
        g_playerUnits.clear(); g_enemyUnits.clear();
        resizeBattleConfig(cfg);
        for(int t=0;t<unitTypeCount();t++){
            for(int i=0;i<cfg.playerCounts[t];i++) g_playerUnits.push_back({t});
            for(int i=0;i<cfg.enemyCounts[t];i++)  g_enemyUnits.push_back({t});
        }
        g_customInit=true;
    }

    ClearBackground({8,12,8,255});
    for(int gx=0;gx<SCREEN_W;gx+=40) DrawLine(gx,0,gx,SCREEN_H,{20,32,20,60});
    for(int gy=0;gy<SCREEN_H;gy+=40) DrawLine(0,gy,SCREEN_W,gy,{20,32,20,60});

    // ── Cabecera ──────────────────────────────────────────────
    DrawRectangle(0,0,SCREEN_W,52,{0,0,0,220});
    DrawText("CUSTOM BATTLE",16,10,26,GREEN);
    DrawText("Configura los ejercitos antes de la batalla",16,38,13,{100,160,100,255});

    // ── ZONA SUPERIOR: dos columnas ejércitos ─────────────────
    // Layout: cabecera(52) | ejércitos(topH) | unidades(unitH) | barra inferior fija(bottomBarH)
    int bottomBarH = 60;  // zona reservada para dificultad + botones
    int topH = 340;
    int unitAreaY = 52 + topH + 6;
    int unitAreaH = SCREEN_H - unitAreaY - bottomBarH;

    float colW=(SCREEN_W-30)/2.0f;
    int pRemove=drawArmyList(g_playerUnits,10,52,colW-5,topH,
                              {60,120,220,255},{80,160,255,255},"NUESTRO EJERCITO",mouse,true);
    if(pRemove>=0){ g_playerUnits.erase(g_playerUnits.begin()+pRemove); syncCounts(cfg); }

    float rightX=colW+20;
    int eRemove=drawArmyList(g_enemyUnits,rightX,52,colW-10,topH,
                              {200,50,50,255},{255,100,100,255},"EJERCITO ENEMIGO (IA)",mouse,false);
    if(eRemove>=0){ g_enemyUnits.erase(g_enemyUnits.begin()+eRemove); syncCounts(cfg); }

    // ── ZONA MEDIA: tipos de unidades disponibles ─────────────
    float bY=(float)unitAreaY;
    DrawRectangle(0,(int)bY,SCREEN_W,unitAreaH,{10,18,10,220});
    DrawLine(0,(int)bY,SCREEN_W,(int)bY,{60,100,60,180});
    DrawText("UNIDADES DISPONIBLES",16,(int)(bY+8),15,{120,200,120,255});

    int nc=unitTypeCount();
    float cardW=(SCREEN_W-40.0f)/nc;
    float cardX=20, cardY=bY+32;
    float cardH=(float)unitAreaH-40;

    for(int i=0;i<nc;i++){
        const UnitTypeDef& td=g_unitTypes[i];
        DrawRectangle((int)cardX,(int)cardY,(int)(cardW-8),(int)cardH,{14,24,14,220});
        DrawRectangleLinesEx({cardX,cardY,cardW-8,cardH},1,{50,90,50,255});

        Color typeCol={td.r,td.g,td.b,255};
        DrawRectangle((int)(cardX+8),(int)(cardY+8),(int)(cardW-24),18,typeCol);
        int nw=MeasureText(td.name,14);
        DrawText(td.name,(int)(cardX+(cardW-8)/2-nw/2),(int)(cardY+10),14,{10,10,10,255});

        float sy2=cardY+34;
        float dps=td.damage/td.attackCooldown;
        DrawText(TextFormat("HP:  %.0f",td.hp),           (int)(cardX+8),(int)sy2,12,{60,220,60,255});   sy2+=15;
        DrawText(TextFormat("DMG: %.0f",td.damage),        (int)(cardX+8),(int)sy2,12,{220,160,40,255});  sy2+=15;
        DrawText(TextFormat("Vel: %.0f",td.speed),         (int)(cardX+8),(int)sy2,12,{60,180,220,255});  sy2+=15;
        DrawText(TextFormat("Rng: %.0f",td.attackRange),   (int)(cardX+8),(int)sy2,12,{200,200,80,255});  sy2+=15;
        DrawText(TextFormat("DPS: %.1f",dps),              (int)(cardX+8),(int)sy2,12,YELLOW);            sy2+=18;

        float btnW=cardW-20, btnX=cardX+6;
        // + Nuestro (azul)
        Rectangle rp={btnX,sy2,btnW,24};
        bool pvHv=CheckCollisionPointRec(mouse,rp);
        DrawRectangleRec(rp,pvHv?Color{40,80,180,255}:Color{20,50,120,220});
        DrawRectangleLinesEx(rp,1,pvHv?Color{100,160,255,255}:Color{50,100,200,255});
        int tpw=MeasureText("+ Nuestro",13);
        DrawText("+ Nuestro",(int)(btnX+btnW/2-tpw/2),(int)(sy2+5),13,pvHv?WHITE:Color{160,200,255,255});
        if(pvHv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){ g_playerUnits.push_back({i}); syncCounts(cfg); }
        sy2+=30;
        // + Enemigo (rojo)
        Rectangle re2={btnX,sy2,btnW,24};
        bool evHv=CheckCollisionPointRec(mouse,re2);
        DrawRectangleRec(re2,evHv?Color{160,30,30,255}:Color{100,15,15,220});
        DrawRectangleLinesEx(re2,1,evHv?Color{255,100,100,255}:Color{180,50,50,255});
        int tew=MeasureText("+ Enemigo",13);
        DrawText("+ Enemigo",(int)(btnX+btnW/2-tew/2),(int)(sy2+5),13,evHv?WHITE:Color{255,160,160,255});
        if(evHv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){ g_enemyUnits.push_back({i}); syncCounts(cfg); }

        cardX+=cardW;
    }

    // ── BARRA INFERIOR FIJA: Dificultad + Botones (sin solapamiento) ──
    int barY=SCREEN_H-bottomBarH;
    DrawRectangle(0,barY,SCREEN_W,bottomBarH,{0,0,0,200});
    DrawLine(0,barY,SCREEN_W,barY,{60,100,60,160});

    // Zona dificultad (izquierda)
    Color dcol=cfg.difficulty<0.8f?Color{60,200,60,200}:
               cfg.difficulty<1.2f?Color{200,200,60,200}:
               cfg.difficulty<1.6f?Color{220,130,40,200}:Color{220,50,50,200};
    const char* dlbl=cfg.difficulty<0.8f?"Facil":cfg.difficulty<1.2f?"Normal":
                     cfg.difficulty<1.6f?"Dificil":"EXTREMO";
    Color dclr=cfg.difficulty<0.8f?GREEN:cfg.difficulty<1.2f?YELLOW:
               cfg.difficulty<1.6f?ORANGE:RED;

    DrawText("Dificultad:"  ,16,barY+10,13,LIGHTGRAY);
    cfg.difficulty=drawSlider({16,(float)(barY+26),380,16},cfg.difficulty,0.5f,2.0f,
                              nullptr,"%.2fx",mouse,dcol);
    DrawText(TextFormat("%.2fx",cfg.difficulty),402,barY+28,13,WHITE);
    DrawText(dlbl,460,barY+28,13,dclr);

    // Zona botones (derecha)
    int pTotal=totalUnits(cfg.playerCounts);
    int eTotal=totalUnits(cfg.enemyCounts);
    bool canStart=(pTotal>0&&eTotal>0);
    Color startCN=canStart?Color{30,70,30,255}:Color{35,35,35,255};
    Color startCH=canStart?Color{60,140,60,255}:Color{35,35,35,255};

    if(drawButton({(float)(SCREEN_W-420),barY+10,280,38},"INICIAR BATALLA",mouse,startCN,startCH)&&canStart){
        g_customInit=false;
        return STATE_PLAYING;
    }
    if(!canStart)
        DrawText("Añade unidades a ambos lados",(int)(SCREEN_W-420),(int)(barY+52),12,{200,100,50,255});
    if(drawButton({(float)(SCREEN_W-130),barY+10,118,38},"VOLVER",mouse,{40,25,25,255},{80,40,40,255})){
        g_customInit=false;
        return STATE_MENU;
    }

    return STATE_CUSTOM_BATTLE;
}

// ═══════════════════════════════════════════════════════════════
//  EDITOR DE UNIDADES
//  — Panel izquierdo: dropdown tipo + sliders stats + color
//  — Panel derecho:   preview solo del tipo seleccionado + stats
//  — Sin "Army a editar"
//  — Botón "Crear nuevo tipo" para añadir tipos custom
// ═══════════════════════════════════════════════════════════════

// Estado persistente del editor
static int    g_editTypeIdx=0;
static bool   g_dropdownOpen=false;
static bool   g_editTexDirty=false;
// Estado para crear nuevo tipo
static bool   g_creatingNew=false;
static UnitTypeDef g_newTypeDraft;
static char   g_newTypeName[32]="NuevoTipo";
static int    g_nameEditActive=0; // 1=nombre activo para edición

static Texture2D g_previewTex={0};  // textura de preview del tipo activo

void rebuildPreviewTex(){
    if(g_previewTex.id>0) UnloadTexture(g_previewTex);
    if(g_editTypeIdx<unitTypeCount()){
        const UnitTypeDef& td=g_unitTypes[g_editTypeIdx];
        Image img;
        switch(td.spriteBase){
            case SPR_SOLDIER: img=makeSoldierImg(td.r,td.g,td.b); break;
            case SPR_SNIPER:  img=makeSniperImg(td.r,td.g,td.b); break;
            default:          img=makeTankImg(td.r,td.g,td.b); break;
        }
        g_previewTex=imgToTex(img);
    }
}

Texture2D makeDraftPreview(const UnitTypeDef& td){
    Image img;
    switch(td.spriteBase){
        case SPR_SOLDIER: img=makeSoldierImg(td.r,td.g,td.b); break;
        case SPR_SNIPER:  img=makeSniperImg(td.r,td.g,td.b); break;
        default:          img=makeTankImg(td.r,td.g,td.b); break;
    }
    return imgToTex(img);
}

GameState updateDrawUnitEditor(Vector2 mouse){
    if(g_editTypeIdx>=unitTypeCount()) g_editTypeIdx=0;

    ClearBackground({8,12,18,255});
    for(int gx=0;gx<SCREEN_W;gx+=40) DrawLine(gx,0,gx,SCREEN_H,{18,22,38,80});
    for(int gy=0;gy<SCREEN_H;gy+=40) DrawLine(0,gy,SCREEN_W,gy,{18,22,38,80});

    // ── Cabecera ──────────────────────────────────────────────
    DrawRectangle(0,0,SCREEN_W,52,{0,0,0,220});
    DrawText("EDITOR DE UNIDADES",16,10,26,GREEN);
    DrawText("Edita los stats de cada tipo. Los cambios se aplican al instante.",16,38,12,{100,160,100,255});

    // ── Layout: panel izq (sliders) / panel der (preview) ─────
    int headerH=52;
    int panelY=headerH+6;
    int panelH=SCREEN_H-panelY-52;  // 52 abajo para el botón volver
    int leftW=500;
    int rightX=leftW+20;
    int rightW=SCREEN_W-rightX-10;

    // Paneles de fondo
    DrawRectangle(10,panelY,leftW,panelH,{12,18,12,210});
    DrawRectangleLinesEx({10,(float)panelY,(float)leftW,(float)panelH},1,{50,90,50,200});
    DrawRectangle(rightX,panelY,rightW,panelH,{12,12,28,210});
    DrawRectangleLinesEx({(float)rightX,(float)panelY,(float)rightW,(float)panelH},1,{50,50,100,200});

    // ════════════════════════════════════════════════════════════
    //  PANEL IZQUIERDO — selector (dropdown) + sliders
    // ════════════════════════════════════════════════════════════
    float lx=20, ly=(float)panelY+10;

    // ── Dropdown de tipos ──────────────────────────────────────
    DrawText("Tipo de unidad:",lx,ly,14,LIGHTGRAY); ly+=20;
    if(g_editTypeIdx<unitTypeCount()){
        const UnitTypeDef& cur=g_unitTypes[g_editTypeIdx];
        Rectangle dropBtn={lx,ly,(float)(leftW-20),28};
        bool dropHv=CheckCollisionPointRec(mouse,dropBtn);
        DrawRectangleRec(dropBtn,dropHv?Color{30,70,50,255}:Color{18,40,30,255});
        DrawRectangleLinesEx(dropBtn,1,g_dropdownOpen?GREEN:Color{50,90,60,255});
        Color patch={cur.r,cur.g,cur.b,255};
        DrawRectangle((int)lx+4,(int)ly+6,16,16,patch);
        DrawText(cur.name,(int)(lx+26),(int)(ly+6),15,WHITE);
        // Triángulo desplegable
        int tx=(int)(lx+leftW-30), ty=(int)(ly+14);
        DrawTriangle({(float)tx,(float)(ty-5)},{(float)(tx-6),(float)(ty+5)},{(float)(tx+6),(float)(ty+5)},
                     g_dropdownOpen?GREEN:LIGHTGRAY);
        if(dropHv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) g_dropdownOpen=!g_dropdownOpen;

        // Lista desplegable
        if(g_dropdownOpen){
            float oy=ly+30;
            for(int i=0;i<unitTypeCount();i++){
                const UnitTypeDef& td=g_unitTypes[i];
                bool sel=(i==g_editTypeIdx);
                Rectangle opt={lx,oy,(float)(leftW-20),26};
                bool ohv=CheckCollisionPointRec(mouse,opt);
                DrawRectangleRec(opt,sel?Color{30,80,40,255}:ohv?Color{22,50,32,255}:Color{14,30,20,240});
                DrawRectangleLinesEx(opt,1,sel?GREEN:Color{40,70,45,255});
                Color op={td.r,td.g,td.b,255};
                DrawRectangle((int)lx+4,(int)oy+5,14,14,op);
                DrawText(td.name,(int)(lx+24),(int)(oy+5),14,sel?GREEN:WHITE);
                if(ohv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                    g_editTypeIdx=i; g_dropdownOpen=false; g_editTexDirty=true;
                }
                oy+=27;
            }
            // Capturar clic fuera para cerrar
            if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                float listH=27.0f*unitTypeCount();
                if(!CheckCollisionPointRec(mouse,{lx,ly,(float)(leftW-20),28+listH}))
                    g_dropdownOpen=false;
            }
            ly+=(float)(27*unitTypeCount())+36;
        } else {
            ly+=36;
        }
    }

    if(g_editTypeIdx>=unitTypeCount()){ ly+=10; goto skipSliders; }
    {
        UnitTypeDef& td=g_unitTypes[g_editTypeIdx];
        bool changed=false;

        DrawLine((int)lx,(int)ly,(int)(lx+leftW-20),(int)ly,{50,90,50,120}); ly+=8;

        // ── Sliders de stats ───────────────────────────────────
        DrawText("ESTADISTICAS",lx,ly,13,{100,200,100,255}); ly+=18;
        float sw=leftW-70;

        float spd=td.speed;
        spd=drawSlider({lx,ly,sw,18},spd,20,400,"Velocidad","%.0f",mouse,{60,180,220,200});
        if(spd!=td.speed){td.speed=spd; changed=true;} ly+=42;

        float hp=td.hp;
        hp=drawSlider({lx,ly,sw,18},hp,20,600,"Vida (HP)","%.0f",mouse,{60,220,60,200});
        if(hp!=td.hp){td.hp=hp; changed=true;} ly+=42;

        float dmg=td.damage;
        dmg=drawSlider({lx,ly,sw,18},dmg,5,200,"Daño","%.0f",mouse,{220,160,40,200});
        if(dmg!=td.damage){td.damage=dmg; changed=true;} ly+=42;

        float rng=td.attackRange;
        rng=drawSlider({lx,ly,sw,18},rng,40,400,"Rango","%.0f",mouse,{200,200,80,200});
        if(rng!=td.attackRange){td.attackRange=rng; changed=true;} ly+=42;

        float cd=td.attackCooldown;
        cd=drawSlider({lx,ly,sw,18},cd,0.2f,5.0f,"Cadencia (s entre disparos)","%.2f",mouse,{180,80,80,200});
        if(cd!=td.attackCooldown){td.attackCooldown=cd; changed=true;} ly+=48;

        // ── Color del cuerpo ───────────────────────────────────
        DrawLine((int)lx,(int)ly,(int)(lx+leftW-20),(int)ly,{50,90,50,120}); ly+=8;
        DrawText("COLOR DEL CUERPO",lx,ly,13,{100,200,100,255}); ly+=18;

        float nr=(float)td.r;
        nr=drawSlider({lx+18,ly,sw,14},nr,0,255,nullptr,"%.0f",mouse,{220,60,60,200});
        DrawRectangle((int)lx,(int)ly,14,14,{(unsigned char)nr,0,0,255}); ly+=30;

        float ng=(float)td.g;
        ng=drawSlider({lx+18,ly,sw,14},ng,0,255,nullptr,"%.0f",mouse,{60,220,60,200});
        DrawRectangle((int)lx,(int)ly,14,14,{0,(unsigned char)ng,0,255}); ly+=30;

        float nb=(float)td.b;
        nb=drawSlider({lx+18,ly,sw,14},nb,0,255,nullptr,"%.0f",mouse,{60,60,220,200});
        DrawRectangle((int)lx,(int)ly,14,14,{0,0,(unsigned char)nb,255}); ly+=36;

        if((unsigned char)nr!=td.r||(unsigned char)ng!=td.g||(unsigned char)nb!=td.b){
            td.r=(unsigned char)nr; td.g=(unsigned char)ng; td.b=(unsigned char)nb;
            changed=true;
        }
        // Muestra de color
        DrawRectangle((int)lx,(int)ly,50,20,{td.r,td.g,td.b,255});
        DrawRectangleLinesEx({(float)lx,(float)ly,50,20},1,WHITE);
        DrawText(TextFormat("RGB(%d,%d,%d)",td.r,td.g,td.b),(int)(lx+56),(int)(ly+4),13,LIGHTGRAY);

        if(changed){
            rebuildTexture(g_editTypeIdx);
            g_editTexDirty=true;
        }
        if(g_editTexDirty){ rebuildPreviewTex(); g_editTexDirty=false; }
    }
    skipSliders:;

    // ════════════════════════════════════════════════════════════
    //  PANEL DERECHO — preview solo del tipo activo + stats
    // ════════════════════════════════════════════════════════════
    if(g_editTypeIdx<unitTypeCount()){
        const UnitTypeDef& td=g_unitTypes[g_editTypeIdx];
        float px2=(float)rightX, py=(float)panelY, pw=(float)rightW, ph=(float)panelH;

        DrawText(TextFormat("PREVIEW: %s",td.name),(int)(px2+12),(int)(py+12),16,SKYBLUE);
        DrawLine((int)px2,(int)(py+34),(int)(px2+pw),(int)(py+34),{50,50,100,120});

        // Sprite animado centrado
        static float pAngle=0; pAngle+=50.0f*GetFrameTime();
        Vector2 center={px2+pw/2, py+ph*0.38f};
        if(g_previewTex.id>0){
            drawSprite(g_previewTex,center,pAngle,9.0f,{td.r,td.g,td.b,255});
            DrawCircleLines((int)center.x,(int)center.y,60,{100,100,200,80});
        }

        // Stats del tipo activo
        float sy=(float)(py+ph*0.62f);
        float dps=td.damage/td.attackCooldown;
        DrawText("ESTADISTICAS ACTUALES",(int)(px2+12),(int)sy,14,{100,180,255,255}); sy+=22;

        auto statRow=[&](const char* label,float val,Color col){
            DrawText(label,(int)(px2+12),(int)sy,14,LIGHTGRAY);
            DrawText(TextFormat("%.1f",val),(int)(px2+160),(int)sy,14,col);
            sy+=20;
        };
        statRow("Vida (HP):",    td.hp,    {60,220,60,255});
        statRow("Daño:",         td.damage,{220,160,40,255});
        statRow("Velocidad:",    td.speed, {60,180,220,255});
        statRow("Rango:",        td.attackRange,{200,200,80,255});
        statRow("Cadencia (s):", td.attackCooldown,{180,80,80,255});
        DrawText("DPS:",(int)(px2+12),(int)sy,14,LIGHTGRAY);
        DrawText(TextFormat("%.2f",dps),(int)(px2+160),(int)sy,14,YELLOW); sy+=28;

        DrawLine((int)px2,(int)sy,(int)(px2+pw),(int)sy,{50,50,100,120}); sy+=10;

        // Sprite base
        DrawText("Sprite base:",(int)(px2+12),(int)sy,13,LIGHTGRAY);
        DrawText(spriteBaseNames[td.spriteBase],(int)(px2+110),(int)sy,13,GREEN); sy+=20;

        if(td.isBuiltin)
            DrawText("Tipo original (builtin)",(int)(px2+12),(int)sy,12,{120,120,80,255});
        else
            DrawText("Tipo personalizado",(int)(px2+12),(int)sy,12,{100,200,100,255});
    }

    // ════════════════════════════════════════════════════════════
    //  PANEL "CREAR NUEVO TIPO" — aparece superpuesto centrado
    // ════════════════════════════════════════════════════════════
    static Texture2D draftTex={0};

    if(g_creatingNew){
        // Fondo semi-opaco
        DrawRectangle(0,0,SCREEN_W,SCREEN_H,{0,0,0,180});
        float dw=700, dh=540;
        float dx=(SCREEN_W-dw)/2, dy=(SCREEN_H-dh)/2;
        DrawRectangle((int)dx,(int)dy,(int)dw,(int)dh,{12,18,28,245});
        DrawRectangleLinesEx({dx,dy,dw,dh},2,{80,140,220,255});
        DrawText("CREAR NUEVO TIPO DE UNIDAD",(int)(dx+20),(int)(dy+14),18,{80,160,255,255});
        DrawLine((int)dx,(int)(dy+40),(int)(dx+dw),(int)(dy+40),{80,140,220,80});

        float cx=dx+20, cy=dy+54;

        // Nombre (edición sencilla con teclado)
        DrawText("Nombre:",(int)cx,(int)cy,14,LIGHTGRAY); cy+=20;
        Rectangle nameBox={cx,cy,280,26};
        bool nameHv=CheckCollisionPointRec(mouse,nameBox);
        DrawRectangleRec(nameBox,g_nameEditActive?Color{20,40,60,255}:Color{14,26,40,255});
        DrawRectangleLinesEx(nameBox,1,g_nameEditActive?Color{100,180,255,255}:Color{50,90,130,255});
        DrawText(g_newTypeDraft.name,(int)(cx+6),(int)(cy+5),14,WHITE);
        if(g_nameEditActive) DrawText("|",(int)(cx+6+MeasureText(g_newTypeDraft.name,14)),(int)(cy+4),14,WHITE);
        if(nameHv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) g_nameEditActive=1;
        else if(!nameHv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) g_nameEditActive=0;
        if(g_nameEditActive){
            int key=GetCharPressed();
            while(key>0){
                int len=strlen(g_newTypeDraft.name);
                if(key>=32&&len<30){ g_newTypeDraft.name[len]=(char)key; g_newTypeDraft.name[len+1]='\0'; }
                key=GetCharPressed();
            }
            if(IsKeyPressed(KEY_BACKSPACE)){
                int len=strlen(g_newTypeDraft.name);
                if(len>0) g_newTypeDraft.name[len-1]='\0';
            }
        }
        cy+=34;

        // Sprite base selector
        DrawText("Sprite base:",(int)cx,(int)cy,14,LIGHTGRAY); cy+=20;
        for(int s=0;s<SPR_BASE_COUNT;s++){
            bool sel=(g_newTypeDraft.spriteBase==(SpriteBase)s);
            Rectangle sb={cx+s*150.0f,cy,140,26};
            bool shv=CheckCollisionPointRec(mouse,sb);
            DrawRectangleRec(sb,sel?Color{20,60,100,255}:shv?Color{18,40,65,255}:Color{12,25,45,255});
            DrawRectangleLinesEx(sb,1,sel?Color{80,180,255,255}:Color{40,80,120,255});
            int stw=MeasureText(spriteBaseNames[s],14);
            DrawText(spriteBaseNames[s],(int)(cx+s*150+(140-stw)/2),(int)(cy+5),14,sel?WHITE:LIGHTGRAY);
            if(shv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                g_newTypeDraft.spriteBase=(SpriteBase)s;
                // Rebuild draft preview
                if(draftTex.id>0) UnloadTexture(draftTex);
                draftTex=makeDraftPreview(g_newTypeDraft);
            }
        }
        cy+=36;

        // Sliders stats del draft
        float sw2=280;
        DrawText("Estadisticas:",(int)cx,(int)cy,14,{100,200,100,255}); cy+=18;
        float spd2=g_newTypeDraft.speed;
        spd2=drawSlider({cx,cy,sw2,16},spd2,20,400,"Velocidad","%.0f",mouse,{60,180,220,200});
        if(spd2!=g_newTypeDraft.speed){ g_newTypeDraft.speed=spd2; if(draftTex.id>0)UnloadTexture(draftTex); draftTex=makeDraftPreview(g_newTypeDraft); }
        cy+=38;
        float hp2=g_newTypeDraft.hp;
        hp2=drawSlider({cx,cy,sw2,16},hp2,20,600,"Vida (HP)","%.0f",mouse,{60,220,60,200});
        g_newTypeDraft.hp=hp2; cy+=38;
        float dmg2=g_newTypeDraft.damage;
        dmg2=drawSlider({cx,cy,sw2,16},dmg2,5,200,"Daño","%.0f",mouse,{220,160,40,200});
        g_newTypeDraft.damage=dmg2; cy+=38;
        float rng2=g_newTypeDraft.attackRange;
        rng2=drawSlider({cx,cy,sw2,16},rng2,40,400,"Rango","%.0f",mouse,{200,200,80,200});
        g_newTypeDraft.attackRange=rng2; cy+=38;
        float cd2=g_newTypeDraft.attackCooldown;
        cd2=drawSlider({cx,cy,sw2,16},cd2,0.2f,5.0f,"Cadencia","%.2f",mouse,{180,80,80,200});
        g_newTypeDraft.attackCooldown=cd2; cy+=38;

        // Color draft
        float nr2=(float)g_newTypeDraft.r;
        nr2=drawSlider({cx+16,cy,sw2,14},nr2,0,255,"Color R","%.0f",mouse,{220,60,60,200});
        DrawRectangle((int)cx,(int)cy,12,14,{(unsigned char)nr2,0,0,255});
        g_newTypeDraft.r=(unsigned char)nr2; cy+=30;
        float ng2=(float)g_newTypeDraft.g;
        ng2=drawSlider({cx+16,cy,sw2,14},ng2,0,255,"Color G","%.0f",mouse,{60,220,60,200});
        DrawRectangle((int)cx,(int)cy,12,14,{0,(unsigned char)ng2,0,255});
        g_newTypeDraft.g=(unsigned char)ng2; cy+=30;
        float nb2=(float)g_newTypeDraft.b;
        nb2=drawSlider({cx+16,cy,sw2,14},nb2,0,255,"Color B","%.0f",mouse,{60,60,220,200});
        DrawRectangle((int)cx,(int)cy,12,14,{0,0,(unsigned char)nb2,255});
        g_newTypeDraft.b=(unsigned char)nb2; cy+=30;

        // Preview del draft (derecha del diálogo)
        static float dAngle=0; dAngle+=50.0f*GetFrameTime();
        if(draftTex.id>0){
            Vector2 dc={dx+dw-160, dy+200};
            drawSprite(draftTex,dc,dAngle,8.0f,{g_newTypeDraft.r,g_newTypeDraft.g,g_newTypeDraft.b,255});
            DrawCircleLines((int)dc.x,(int)dc.y,52,{80,140,220,80});
        }

        // Botones confirmar/cancelar
        bool nameOk=(strlen(g_newTypeDraft.name)>0);
        Color confCN=nameOk?Color{20,70,20,255}:Color{30,30,30,255};
        Color confCH=nameOk?Color{40,130,40,255}:Color{30,30,30,255};
        if(drawButton({dx+20,dy+dh-50,200,38},"CREAR UNIDAD",mouse,confCN,confCH)&&nameOk){
            g_newTypeDraft.isBuiltin=false;
            g_unitTypes.push_back(g_newTypeDraft);
            int newIdx=(int)g_unitTypes.size()-1;
            rebuildTexture(newIdx);
            g_editTypeIdx=newIdx;
            g_creatingNew=false;
            g_editTexDirty=true;
            if(draftTex.id>0){ UnloadTexture(draftTex); draftTex={0}; }
            rebuildPreviewTex();
        }
        if(drawButton({dx+240,dy+dh-50,160,38},"CANCELAR",mouse,{60,25,25,255},{110,45,45,255})){
            g_creatingNew=false;
            if(draftTex.id>0){ UnloadTexture(draftTex); draftTex={0}; }
        }

    } else {
        // Botón "Nuevo tipo" — en panel izquierdo abajo
        int btnY=SCREEN_H-48;
        if(drawButton({10,(float)btnY,200,36},"+ NUEVA UNIDAD",mouse,{20,50,80,255},{40,90,150,255})){
            memset(&g_newTypeDraft,0,sizeof(g_newTypeDraft));
            strcpy(g_newTypeDraft.name,"NuevoTipo");
            g_newTypeDraft.speed=120; g_newTypeDraft.hp=100;
            g_newTypeDraft.damage=30; g_newTypeDraft.attackRange=120;
            g_newTypeDraft.attackCooldown=1.0f;
            g_newTypeDraft.r=200; g_newTypeDraft.g=80; g_newTypeDraft.b=200;
            g_newTypeDraft.spriteBase=SPR_TANK;
            g_newTypeDraft.isBuiltin=false;
            g_creatingNew=true;
            g_nameEditActive=1;
            if(draftTex.id>0){ UnloadTexture(draftTex); draftTex={0}; }
            draftTex=makeDraftPreview(g_newTypeDraft);
        }
        if(drawButton({220,(float)btnY,160,36},"VOLVER",mouse,{40,25,25,255},{80,40,40,255}))
            return STATE_MENU;
    }

    return STATE_UNIT_EDITOR;
}

// ═══════════════════════════════════════════════════════════════
//  INICIALIZAR PARTIDA
// ═══════════════════════════════════════════════════════════════
void initGame(std::vector<Unit>& units,std::vector<Enemy>& enemies,
              std::vector<Bullet>& bullets, const BattleConfig& cfg){
    units.clear(); enemies.clear(); bullets.clear();

    float startX=60, startY=260, cx=startX;
    for(int t=0;t<unitTypeCount();t++){
        if(t>=(int)cfg.playerCounts.size()) break;
        int cnt=cfg.playerCounts[t];
        if(cnt==0) continue;
        const UnitTypeDef& td=g_unitTypes[t];
        for(int i=0;i<cnt;i++){
            int col=i%3, row=i/3;
            Unit u;
            u.pos={cx+col*36.0f, startY+row*38.0f};
            u.target=u.pos;
            u.hp=u.maxHp       = td.hp;
            u.damage           = td.damage;
            u.speed            = td.speed;
            u.attackRange      = td.attackRange;
            u.attackCooldown   = td.attackCooldown;
            u.selected=u.moving=false; u.alive=true;
            u.angle=0; u.angleTarget=0; u.attackTimer=0;
            u.bodyColor={td.r,td.g,td.b,255};
            u.typeIdx=t;
            units.push_back(u);
        }
        cx+=130;
    }

    float ex=SCREEN_W-70, ey=200, ecx=ex;
    for(int t=0;t<unitTypeCount();t++){
        if(t>=(int)cfg.enemyCounts.size()) break;
        int cnt=cfg.enemyCounts[t];
        if(cnt==0) continue;
        const UnitTypeDef& td=g_unitTypes[t];
        for(int i=0;i<cnt;i++){
            int col=i%3, row=i/3;
            Enemy e;
            e.pos={ecx-col*36.0f, ey+row*38.0f};
            e.hp=e.maxHp = td.hp    * cfg.difficulty;
            e.speed      = td.speed * cfg.difficulty;
            e.alive=true; e.attackTimer=0; e.angle=180.0f;
            e.typeIdx=t;
            enemies.push_back(e);
        }
        ecx-=130;
    }
}

// ═══════════════════════════════════════════════════════════════
//  PARTIDA
// ═══════════════════════════════════════════════════════════════
GameState updateDrawGame(std::vector<Unit>& units,std::vector<Enemy>& enemies,
                         std::vector<Bullet>& bullets,
                         const BattleConfig& cfg,
                         Vector2 mouse,float dt){

    static Vector2   selStart={0,0};
    static bool      dragging=false;
    static Rectangle selRect={0,0,0,0};

    bool menuPressed=drawButton({(float)(SCREEN_W-130),6,120,30},"MENU",mouse,
                                {30,50,30,220},{60,110,60,255});
    if(menuPressed||IsKeyPressed(KEY_ESCAPE)) return STATE_MENU;
    bool overMenuBtn=btnHover({(float)(SCREEN_W-130),6,120,30},mouse);

    if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)&&!overMenuBtn){
        selStart=mouse; dragging=true;
        for(auto& u:units) u.selected=false;
    }
    if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)&&dragging){
        selRect.x=fminf(mouse.x,selStart.x); selRect.y=fminf(mouse.y,selStart.y);
        selRect.width=fabsf(mouse.x-selStart.x); selRect.height=fabsf(mouse.y-selStart.y);
    }
    if(IsMouseButtonReleased(MOUSE_LEFT_BUTTON)){
        dragging=false;
        if(selRect.width<5&&selRect.height<5){
            float best=9999; int idx=-1;
            for(int i=0;i<(int)units.size();i++){
                if(!units[i].alive) continue;
                float d=vdist(mouse,units[i].pos);
                if(d<best&&d<22){best=d;idx=i;}
            }
            if(idx>=0) units[idx].selected=true;
        } else {
            for(auto& u:units)
                if(u.alive&&CheckCollisionPointRec(u.pos,selRect)) u.selected=true;
        }
        selRect={0,0,0,0};
    }
    if(IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)){
        int idx=0;
        for(auto& u:units){
            if(!u.selected) continue;
            u.target={mouse.x+(idx%3-1)*32.0f, mouse.y+(idx/3-1)*32.0f};
            u.moving=true; idx++;
        }
    }

    // ── Lógica unidades ──────────────────────────────────────
    for(auto& u:units){
        if(!u.alive) continue;
        u.attackTimer-=dt;
        if(u.moving){
            float d=vdist(u.pos,u.target);
            if(d<3.0f) u.moving=false;
            else{
                Vector2 dir=vnorm({u.target.x-u.pos.x,u.target.y-u.pos.y});
                u.pos.x+=dir.x*u.speed*dt; u.pos.y+=dir.y*u.speed*dt;
                u.angleTarget=dirToAngle(dir);
            }
        }
        Enemy* best=nullptr; float bestD=9999;
        for(auto& e:enemies){
            if(!e.alive) continue;
            float d=vdist(u.pos,e.pos);
            if(d<u.attackRange+20&&d<bestD){bestD=d;best=&e;}
        }
        if(best){
            Vector2 dir=vnorm({best->pos.x-u.pos.x,best->pos.y-u.pos.y});
            u.angleTarget=dirToAngle(dir);
            if(u.attackTimer<=0){
                bullets.push_back({u.pos,{dir.x*320,dir.y*320},u.damage,true,true});
                u.attackTimer=u.attackCooldown;
            }
        }
        u.angle=lerpAngle(u.angle,u.angleTarget,12.0f*dt);
    }

    // ── Lógica enemigos ──────────────────────────────────────
    for(auto& e:enemies){
        if(!e.alive) continue;
        e.attackTimer-=dt;
        int ti=e.typeIdx;
        if(ti>=unitTypeCount()) continue;
        const UnitTypeDef& td=g_unitTypes[ti];
        float eDmg=td.damage*cfg.difficulty, eCd=td.attackCooldown, eRng=td.attackRange;

        Unit* tgt=nullptr; float minD=9999;
        for(auto& u:units){if(!u.alive)continue;float d=vdist(e.pos,u.pos);if(d<minD){minD=d;tgt=&u;}}
        if(tgt){
            Vector2 dir=vnorm({tgt->pos.x-e.pos.x,tgt->pos.y-e.pos.y});
            e.angle=lerpAngle(e.angle,dirToAngle(dir),8.0f*dt);
            if(minD>40) e.pos={e.pos.x+dir.x*e.speed*dt,e.pos.y+dir.y*e.speed*dt};
            if(minD<eRng+40&&e.attackTimer<=0){
                bullets.push_back({e.pos,{dir.x*260,dir.y*260},eDmg,true,false});
                e.attackTimer=eCd;
            }
        }
    }

    // ── Balas ────────────────────────────────────────────────
    for(auto& b:bullets){
        if(!b.alive) continue;
        b.pos.x+=b.vel.x*dt; b.pos.y+=b.vel.y*dt;
        if(b.pos.x<0||b.pos.x>SCREEN_W||b.pos.y<0||b.pos.y>SCREEN_H){b.alive=false;continue;}
        if(b.fromPlayer){
            for(auto& e:enemies){if(!e.alive||!b.alive)continue;
                if(vdist(b.pos,e.pos)<14){e.hp-=b.damage;b.alive=false;if(e.hp<=0)e.alive=false;break;}}
        } else {
            for(auto& u:units){if(!u.alive||!b.alive)continue;
                if(vdist(b.pos,u.pos)<14){u.hp-=b.damage;b.alive=false;if(u.hp<=0)u.alive=false;break;}}
        }
    }

    int au=0,ae=0;
    for(auto& u:units) if(u.alive) au++;
    for(auto& e:enemies) if(e.alive) ae++;

    // ── DIBUJO ───────────────────────────────────────────────
    ClearBackground({20,30,20,255});
    for(int x=0;x<SCREEN_W;x+=64) DrawLine(x,0,x,SCREEN_H,{30,45,30,255});
    for(int y=0;y<SCREEN_H;y+=64) DrawLine(0,y,SCREEN_W,y,{30,45,30,255});

    for(auto& b:bullets){if(!b.alive)continue;
        DrawCircleV(b.pos,4,b.fromPlayer?YELLOW:ORANGE);}

    for(auto& u:units){
        if(!u.alive) continue;
        if(u.selected) DrawCircleLines((int)u.pos.x,(int)u.pos.y,26,SKYBLUE);
        int ti=u.typeIdx;
        if(ti>=unitTypeCount()) continue;
        Color tint=u.selected?WHITE:Color{clampU8(u.bodyColor.r+20),
                                          clampU8(u.bodyColor.g+20),
                                          clampU8(u.bodyColor.b+20),255};
        drawSprite(g_playerTextures[ti],u.pos,u.angle,1.8f,tint);
        drawHealthBar(u.pos,u.hp,u.maxHp,28);
    }
    for(auto& e:enemies){
        if(!e.alive) continue;
        int ti=e.typeIdx;
        if(ti>=unitTypeCount()) continue;
        drawSprite(g_enemyTextures[ti],e.pos,e.angle,1.8f,WHITE);
        drawHealthBar(e.pos,e.hp,e.maxHp,28);
    }
    if(dragging&&(selRect.width>5||selRect.height>5)){
        DrawRectangleRec(selRect,{100,200,255,40});
        DrawRectangleLinesEx(selRect,1,SKYBLUE);
    }

    DrawRectangle(0,0,SCREEN_W,44,{0,0,0,210});
    DrawText(TextFormat("Unidades: %d  |  Enemigos: %d  |  Dif: %.1fx",au,ae,cfg.difficulty),12,13,16,WHITE);
    drawButton({(float)(SCREEN_W-130),6,120,30},"MENU",mouse,{30,50,30,220},{60,110,60,255});

    if(au==0){
        DrawRectangle(0,0,SCREEN_W,SCREEN_H,{0,0,0,170});
        const char* go="GAME OVER";
        DrawText(go,SCREEN_W/2-MeasureText(go,56)/2+2,SCREEN_H/2-30+2,56,{150,0,0,255});
        DrawText(go,SCREEN_W/2-MeasureText(go,56)/2,  SCREEN_H/2-30,  56,RED);
        if(drawButton({SCREEN_W/2-110.0f,(float)(SCREEN_H/2+40),220,44},"VOLVER AL MENU",mouse))
            return STATE_MENU;
    }
    if(ae==0&&au>0){
        DrawRectangle(0,0,SCREEN_W,SCREEN_H,{0,0,0,150});
        const char* win="VICTORIA!";
        DrawText(win,SCREEN_W/2-MeasureText(win,56)/2+2,SCREEN_H/2-30+2,56,{0,120,0,255});
        DrawText(win,SCREEN_W/2-MeasureText(win,56)/2,  SCREEN_H/2-30,  56,GREEN);
        if(drawButton({SCREEN_W/2-110.0f,(float)(SCREEN_H/2+40),220,44},"VOLVER AL MENU",mouse))
            return STATE_MENU;
    }

    return STATE_PLAYING;
}

// ═══════════════════════════════════════════════════════════════
//  MAIN
// ═══════════════════════════════════════════════════════════════
int main(){
    InitWindow(SCREEN_W,SCREEN_H,"RTS Pixel War");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    initBuiltinTypes();
    rebuildPreviewTex();

    GameState    state=STATE_MENU;
    BattleConfig cfg  =defaultBattleConfig();

    std::vector<Unit>   units;
    std::vector<Enemy>  enemies;
    std::vector<Bullet> bullets;
    float menuTime=0;

    while(!WindowShouldClose()){
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
                float bx=SCREEN_W/2-155.0f;
                if(btnHover({bx,474,310,52},mouse)&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                    goto quit;
                if(nxt==STATE_PLAYING){
                    cfg=defaultBattleConfig();
                    initGame(units,enemies,bullets,cfg);
                }
                if(nxt!=STATE_MENU) state=nxt;
                break;
            }
            case STATE_CUSTOM_BATTLE:{
                GameState nxt=updateDrawCustomBattle(cfg,mouse);
                if(nxt==STATE_PLAYING){
                    resizeBattleConfig(cfg);
                    initGame(units,enemies,bullets,cfg);
                    state=STATE_PLAYING;
                } else state=nxt;
                break;
            }
            case STATE_UNIT_EDITOR:{
                state=updateDrawUnitEditor(mouse);
                break;
            }
            case STATE_PLAYING:{
                state=updateDrawGame(units,enemies,bullets,cfg,mouse,dt);
                break;
            }
        }
        EndDrawing();
    }
    quit:
    // Limpiar texturas dinámicas
    for(auto& t:g_playerTextures) UnloadTexture(t);
    for(auto& t:g_enemyTextures)  UnloadTexture(t);
    if(g_previewTex.id>0) UnloadTexture(g_previewTex);
    CloseWindow();
    return 0;
}
