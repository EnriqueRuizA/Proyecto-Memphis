// tools/preview_sprites.cpp — Renderiza assets/sprites/ a build/sprite_preview.png
// sin arrancar el juego (bucle de verificación visual de la integración Kenney).
// ESPEJO A PROPOSITO de roleFor()/drawSheetSprite()/drawPennant() en src/sprite.cpp:
// si cambia la logica ahi, mantener este tool sincronizado.
#include "raylib.h"
#include <cmath>
#include <cstdio>

struct U { const char* name; int sb; int wh; };

static int roleFor(int sb,int wh){
    if(sb==2) return 4; // ranged -> archer
    if(sb==1) return 1; // cavalry -> sword (a pie)
    switch(wh){
        case 0:  return 0; // spear
        case 1:  return 2; // axe
        case 3:  return 3; // coronado
        default: return 1; // sword
    }
}

static void drawSheet(Texture2D tex,float cx,float cy,float angle,float scale,
                      bool bright,bool shadow){
    float s=32.f*scale;
    if(shadow) DrawCircleV({cx+2,cy+4},(int)(s*0.3f),Color{0,0,0,50});
    bool flip=cosf(angle*DEG2RAD)<0.f;
    Rectangle src={0,0,flip?-(float)tex.width:(float)tex.width,(float)tex.height};
    DrawTexturePro(tex,src,{cx,cy,s,s},{s/2,s/2},0.f,
                   bright?WHITE:Color{210,210,210,255});
}

static void pennant(Vector2 pos,float s,int team){
    float px=pos.x+s*0.17f;
    float top=pos.y-s*0.46f, base=pos.y-s*0.18f;
    DrawLineEx({px,top},{px,base},1.2f,{60,45,30,255});
    float wave=sinf(GetTime()*5.f+pos.x*0.11f)*s*0.045f;
    Color c=team?Color{228,80,62,255}:Color{96,158,236,255};
    Vector2 a={px,top}, b={px+s*0.26f+wave,top+s*0.07f}, d={px,top+s*0.15f};
    DrawTriangle(a,b,d,c);
    DrawTriangle(a,d,b,c);
}

int main(){
    const char* pref[2]={"p_","e_"};
    U units[8]={
        {"Spear Levy",0,0},{"Axe Militia",0,1},{"Poleaxe Retinue",0,2},
        {"Dismounted Knights",0,3},{"Bow Levy",2,0},{"Crossbow Retinue",2,0},
        {"Mounted Sergeants",1,0},{"Knights",1,0}
    };

    SetTraceLogLevel(LOG_NONE);
    InitWindow(1400,760,"preview");
    SetTargetFPS(60);

    const char* role[5]={"spear","sword","axe","priest","archer"};
    Texture2D tex[2][5];
    for(int t=0;t<2;t++)
        for(int r=0;r<5;r++){
            char path[128];
            snprintf(path,sizeof(path),"assets/sprites/%s%s.png",pref[t],role[r]);
            if(!FileExists(path)){ printf("FALTA %s\n",path); CloseWindow(); return 1; }
            tex[t][r]=LoadTexture(path);
            SetTextureFilter(tex[t][r],TEXTURE_FILTER_BILINEAR);
        }

    int frame=0;
    while(!WindowShouldClose()){
        BeginDrawing();
        ClearBackground({52,74,48,255});

        DrawText("Kenney Medieval RTS (CC0) — 8 unidades x [aliado | enemigo] x [mirando Este | Oeste]",
                 14,10,16,Color{200,165,80,255});
        DrawText("ALIADO E",180,34,11,GRAY); DrawText("ALIADO O",320,34,11,GRAY);
        DrawText("ENEMIGO E",460,34,11,GRAY); DrawText("ENEMIGO O",600,34,11,GRAY);
        DrawText("escala batalla real",820,34,11,GRAY);

        for(int u=0;u<8;u++){
            int y=70+u*76;
            int ro=roleFor(units[u].sb,units[u].wh);
            bool cav=(units[u].sb==1);
            float sc=1.6f; if(cav) sc*=1.18f;   // misma formula que drawUnit()
            float sPenn=32.f*sc;
            DrawText(units[u].name,14,y+24,13,WHITE);
            drawSheet(tex[0][ro],230,y+38,0.f,sc,true,true);
            if(cav) pennant({230,(float)(y+38)},sPenn,0);
            drawSheet(tex[0][ro],370,y+38,180.f,sc,true,true);
            if(cav) pennant({370,(float)(y+38)},sPenn,0);
            drawSheet(tex[1][ro],510,y+38,0.f,sc,true,true);
            if(cav) pennant({510,(float)(y+38)},sPenn,1);
            drawSheet(tex[1][ro],650,y+38,180.f,sc,true,true);
            if(cav) pennant({650,(float)(y+38)},sPenn,1);
            // escala real de batalla (0.65 x 1.35, caballeria x1.18)
            float sc2=0.65f*1.35f; if(cav) sc2*=1.18f;
            float mx=880.f+(float)(u%4)*44.f;
            float my=(float)(y+20+(u/4)*34);
            drawSheet(tex[0][ro],mx,my,0.f,sc2,true,true);
            if(cav) pennant({mx,my},32.f*sc2,0);
        }
        DrawText("Escala real = filas inferiores (4+4) — fondo del campo de batalla",
                 14,742-16,11,GRAY);

        // Panel de detalle: caballeria grande con banderin (aliado E / enemigo O)
        DrawText("DETALLE - caballeria (banderin)",1120,60,13,Color{200,165,80,255});
        drawSheet(tex[0][1],1180.f,240.f,0.f,5.f,true,true);
        pennant({1180.f,240.f},32.f*5.f,0);
        DrawText("aliado E",1150,360,11,GRAY);
        drawSheet(tex[1][1],1330.f,240.f,180.f,5.f,true,true);
        pennant({1330.f,240.f},32.f*5.f,1);
        DrawText("enemigo O",1300,360,11,GRAY);
        // Infanteria grande para comparar (aliado E)
        drawSheet(tex[0][0],1180.f,540.f,0.f,5.f,true,true);
        DrawText("lancer o",1150,660,11,GRAY);
        drawSheet(tex[1][3],1330.f,540.f,180.f,5.f,true,true);
        DrawText("elite enem.",1295,660,11,GRAY);

        EndDrawing();
        if(++frame>=3){
            Image img=LoadImageFromScreen();
            ExportImage(img,"build/sprite_preview.png");
            UnloadImage(img);
            printf("OK build/sprite_preview.png\n");
            break;
        }
    }
    CloseWindow();
    return 0;
}
