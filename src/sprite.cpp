#include "sprite.h"
#include "config.h"
#include "util.h"
#include "ui.h"
#include "elements.h"
#include <vector>
#include <string>
#include <deque>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <random>
#include <functional>
#include <cassert>
#include <utility>

// pixel helpers for 32x32 sprites
void px32(Image& img,int x,int y,Color c){
    if(x<0||y<0||x>=img.width||y>=img.height)return;
    ImageDrawPixel(&img,x,y,c);
}
void fillRect32(Image& img,int x,int y,int w,int h,Color c){
    for(int yy=y;yy<y+h;yy++)for(int xx=x;xx<x+w;xx++)px32(img,xx,yy,c);
}
void circle32(Image& img,int cx,int cy,int r,Color c,bool fill=true){
    for(int dy=-r;dy<=r;dy++){
        for(int dx=-r;dx<=r;dx++){
            int d2=dx*dx+dy*dy;
            if(fill?(d2<=r*r):((d2>=(r-1)*(r-1))&&(d2<=r*r)))
                px32(img,cx+dx,cy+dy,c);
        }
    }
}

// Build infantry 32x32 sprite (geometric, semi-realistic)
Image makeInfantrySprite(unsigned char r,unsigned char g,unsigned char b,int weaponHint){
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

Image makeCavalrySprite(unsigned char r,unsigned char g,unsigned char b){
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

Image makeRangedSprite(unsigned char r,unsigned char g,unsigned char b,bool crossbow){
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

Texture2D imageToTex(Image img){
    Texture2D tex=LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(tex,TEXTURE_FILTER_BILINEAR);
    return tex;
}

void rebuildTexture(int idx){
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

// ── Hojas externas (Kenney "Medieval RTS", CC0) ─────────────────────────────
// 5 roles x 2 equipos en assets/sprites/. La caballería va a pie (no hay
// caballos en el pack): se distingue por tamaño extra y banderín de equipo.
static bool      s_sheetsLoaded=false;
static Texture2D s_sheets[2][5]; // [equipo][rol]

static int sheetRoleFor(const UnitTypeDef& td){
    if(td.spriteBase==SPR_RANGED)  return 4; // archer
    if(td.spriteBase==SPR_CAVALRY) return 1; // jinete a pie
    switch(td.weaponHint){
        case 0:  return 0; // spear
        case 1:  return 2; // axe
        case 3:  return 3; // coronado (Dismounted Knights)
        default: return 1; // sword
    }
}

static void freeSheets(){
    for(int t=0;t<2;t++)
        for(int r=0;r<5;r++)
            if(s_sheets[t][r].id>0){UnloadTexture(s_sheets[t][r]);s_sheets[t][r]={0};}
    s_sheetsLoaded=false;
}

bool loadUnitSheets(){
    if(s_sheetsLoaded) return true;
    static const char* pref[2]={"p_","e_"};
    static const char* role[5]={"spear","sword","axe","priest","archer"};
    for(int t=0;t<2;t++){
        for(int r=0;r<5;r++){
            char path[128];
            snprintf(path,sizeof(path),"assets/sprites/%s%s.png",pref[t],role[r]);
            if(!FileExists(path)){
                TraceLog(LOG_INFO,"[sprites] falta %s — fallback procedural",path);
                freeSheets();
                return false;
            }
            Texture2D tx=LoadTexture(path);
            if(tx.id==0){
                TraceLog(LOG_WARNING,"[sprites] error cargando %s — fallback procedural",path);
                freeSheets();
                return false;
            }
            SetTextureFilter(tx,TEXTURE_FILTER_BILINEAR);
            s_sheets[t][r]=tx;
        }
    }
    s_sheetsLoaded=true;
    TraceLog(LOG_INFO,"[sprites] hojas Kenney cargadas desde assets/sprites/");
    return true;
}

void     unloadUnitSheets(){ freeSheets(); }
bool     unitSheetsActive(){ return s_sheetsLoaded; }

Texture2D unitTexture(int typeIdx,int team){
    if(s_sheetsLoaded) return s_sheets[team?1:0][sheetRoleFor(g_unitTypes[typeIdx])];
    return team? g_enemyTextures[typeIdx] : g_playerTextures[typeIdx];
}

static void drawPennant(Vector2 pos,float s,int team){
    float px=pos.x+s*0.17f;
    float top=pos.y-s*0.46f, base=pos.y-s*0.18f;
    DrawLineEx({px,top},{px,base},1.2f,{60,45,30,255});
    float wave=sinf(g_menuTime*5.f+pos.x*0.11f)*s*0.045f;
    Color c=team?Color{228,80,62,255}:Color{96,158,236,255};
    Vector2 a={px,top}, b={px+s*0.26f+wave,top+s*0.07f}, d={px,top+s*0.15f};
    DrawTriangle(a,b,d,c);
    DrawTriangle(a,d,b,c); // garantiza visibilidad ante culling de winding
}

static void drawSheetSprite(Texture2D tex,Vector2 pos,float angleDeg,float scale,
                            Color tint,bool shadow){
    float s=32.f*scale;
    if(shadow) DrawCircleV({pos.x+2,pos.y+4},(int)(s*0.3f),Color{0,0,0,50});
    bool flip=cosf(angleDeg*DEG2RAD)<0.f;
    Rectangle src={0,0,flip?-(float)tex.width:(float)tex.width,(float)tex.height};
    DrawTexturePro(tex,src,{pos.x,pos.y,s,s},{s/2,s/2},0.f,tint);
}

void drawUnit(int typeIdx,int team,Vector2 pos,float angleDeg,float scale,
              bool bright,bool shadow){
    const UnitTypeDef& td=g_unitTypes[typeIdx];
    if(s_sheetsLoaded){
        float sc=scale*1.35f;                        // los lienzos son64px con margen
        if(td.spriteBase==SPR_CAVALRY) sc*=1.18f;    // caballería: un punto mayor
        Color tint=bright?WHITE:Color{210,210,210,255};
        drawSheetSprite(s_sheets[team?1:0][sheetRoleFor(td)],pos,angleDeg,sc,tint,shadow);
        if(td.spriteBase==SPR_CAVALRY) drawPennant(pos,32.f*sc,team);
    }else{
        Color tint=team?WHITE:(bright?WHITE:Color{td.r,td.g,td.b,255});
        drawSoldierSprite(team?g_enemyTextures[typeIdx]:g_playerTextures[typeIdx],
                          pos,angleDeg,scale,tint,shadow);
    }
}
