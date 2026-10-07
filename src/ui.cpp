#include "ui.h"
#include "config.h"
#include "util.h"
#include "ui.h"
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

bool drawButton(Rectangle r,const char* lbl,Vector2 m,
                       Color cn,Color ch){
    bool hv=ptInRect(m,r);
    DrawRectangleRec(r,hv?ch:cn);
    DrawRectangleLinesEx(r,2,hv?C_GOLD:Color{80,110,60,255});
    int fs=18;
    int sfs=uiFS(fs);
    int maxH=(int)r.height-6; if(maxH<8) maxH=8;
    if(sfs>maxH) sfs=maxH;
    int tw=MeasureTextRaw(lbl,sfs);
    int maxW=(int)r.width-10; if(maxW<10) maxW=10;
    while(sfs>8 && tw>maxW){ sfs--; tw=MeasureTextRaw(lbl,sfs); }
    DrawTextRaw(lbl,(int)(r.x+r.width/2-tw/2),(int)(r.y+r.height/2-sfs/2),sfs,hv?C_PARCHMENT:C_SECONDARY);
    return hv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

bool drawSmBtn(Rectangle r,const char* lbl,Vector2 m,
                      Color cn,Color ch){
    bool hv=ptInRect(m,r);
    DrawRectangleRec(r,hv?ch:cn);
    DrawRectangleLinesEx(r,1,hv?C_GOLD:Color{60,90,50,255});
    int fs=13;
    int sfs=uiFS(fs);
    int maxH=(int)r.height-6; if(maxH<7) maxH=7;
    if(sfs>maxH) sfs=maxH;
    int tw=MeasureTextRaw(lbl,sfs);
    int maxW=(int)r.width-10; if(maxW<10) maxW=10;
    while(sfs>7 && tw>maxW){ sfs--; tw=MeasureTextRaw(lbl,sfs); }
    DrawTextRaw(lbl,(int)(r.x+r.width/2-tw/2),(int)(r.y+r.height/2-sfs/2),sfs,hv?Color{255,255,255,255}:C_SECONDARY);
    return hv&&IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

int drawIntSlider(Rectangle r,int val,int mn,int mx,
                         const char* label,Vector2 mouse,Color fill){
    if(mx<=mn) return val; // 1.5: guard division by zero
    if(label&&label[0]) DrawText(label,(int)r.x,(int)(r.y-uiPx(15.f)),12,C_SECONDARY);
    DrawRectangleRec(r,{18,18,18,255});
    DrawRectangleLinesEx(r,1,{55,55,55,255});
    float t=(float)(val-mn)/(float)(mx-mn);
    DrawRectangle((int)r.x,(int)r.y,(int)(t*r.width),(int)r.height,fill);
    float kx=r.x+t*r.width;
    float kHalf=uiPx(4.f);
    DrawRectangle((int)(kx-kHalf),(int)(r.y-uiPx(2.f)),(int)(2*kHalf),(int)(r.height+uiPx(4.f)),WHITE);
    DrawText(TextFormat("%d",val),(int)(r.x+r.width+uiPx(6.f)),(int)(r.y+uiPx(1.f)),12,WHITE);
    if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)&&ptInRect(mouse,r)){
        float nt=(mouse.x-r.x)/r.width;
        nt=fmaxf(0.f,fminf(1.f,nt));
        val=mn+(int)roundf(nt*(float)(mx-mn));
    }
    return val;
}

float drawFloatSlider(Rectangle r,float val,float mn,float mx,
                              const char* label,const char* fmt,Vector2 mouse,Color fill){
    if(mx<=mn) return val; // 1.5: guard division by zero
    if(label&&label[0]) DrawText(label,(int)r.x,(int)(r.y-uiPx(15.f)),12,C_SECONDARY);
    DrawRectangleRec(r,{18,18,18,255});
    DrawRectangleLinesEx(r,1,{55,55,55,255});
    float t=(val-mn)/(mx-mn);
    DrawRectangle((int)r.x,(int)r.y,(int)(t*r.width),(int)r.height,fill);
    float kx=r.x+t*r.width;
    float kHalf=uiPx(4.f);
    DrawRectangle((int)(kx-kHalf),(int)(r.y-uiPx(2.f)),(int)(2*kHalf),(int)(r.height+uiPx(4.f)),WHITE);
    DrawText(TextFormat(fmt,val),(int)(r.x+r.width+uiPx(6.f)),(int)(r.y+uiPx(1.f)),12,WHITE);
    if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)&&ptInRect(mouse,r)){
        float nt=(mouse.x-r.x)/r.width;
        nt=fmaxf(0.f,fminf(1.f,nt));
        val=mn+nt*(mx-mn);
    }
    return val;
}

void drawSoldierSprite(Texture2D tex,Vector2 pos,float angleDeg,float scale,Color tint,bool drawShadow){
    float s=32.f*scale;
    if(drawShadow){
        Color shad={0,0,0,50};
        DrawCircleV({pos.x+2,pos.y+4},(int)(s*0.3f),shad);
    }
    DrawTexturePro(tex,{0,0,32,32},{pos.x,pos.y,s,s},{s/2,s/2},-angleDeg+90.f,tint);
}

void drawHealthBar(Vector2 pos,float hp,float maxHp,float w,float scl){
    if(hp>=maxHp) return;
    float bw=w*scl;
    float ratio=hp/maxHp;
    DrawRectangle((int)(pos.x-bw/2),(int)(pos.y-16*scl),(int)bw,(int)(5*scl),DARKGRAY);
    Color c=ratio>0.5f?Color{0,228,48,255}:ratio>0.25f?Color{253,249,0,255}:Color{230,41,55,255};
    DrawRectangle((int)(pos.x-bw/2),(int)(pos.y-16*scl),(int)(bw*ratio),(int)(5*scl),c);
}

// 3.8: Word-wrapped text helper
void drawWrappedText(const char* text,int x,int y,int maxWidth,int fontSize,Color col){
    if(!text||!text[0]) return;
    char word[128]; int wi=0;
    char line[512]; int li=0;
    int curX=0;
    int lineH=uiFS(fontSize)+(int)roundf(uiPx(2.f));
    auto flushLine=[&](){
        line[li]='\0';
        if(li>0){DrawText(line,x,y,fontSize,col);y+=lineH;}
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

