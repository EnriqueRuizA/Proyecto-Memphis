// src/camera.cpp — Cámara de batalla
// Extraído de initBattle (:1458-1463) y updateDrawBattle (:2470-2500).
#include "camera.h"
#include "combat.h"
#include <algorithm>

static Vector2 g_camTarget={BATTLE_W/2.f,BATTLE_H/2.f};
static float   g_zoomTarget=1.f;

void initBattleCamera(){
    // Camera centered on player deployment zone (:1458-1463)
    g_battle.cam.offset={SCREEN_W/2.f,(SCREEN_H-HUD_H)/2.f};
    g_battle.cam.target={BATTLE_W*0.25f,BATTLE_H/2.f};
    g_battle.cam.rotation=0;
    g_battle.cam.zoom=1.f;
    g_battle.camZoom=1.f;
    // Nota: g_camTarget/g_zoomTarget NO se reinician (paridad con los
    // static locales originales dentro de updateDrawBattle).
}

void updateBattleCamera(float dt){
    // Camera pan (arrows / WASD) (:2478-2483)
    float panSpd=400.f/g_battle.cam.zoom;
    if(IsKeyDown(KEY_RIGHT)||IsKeyDown(KEY_D)) g_camTarget.x+=panSpd*dt;
    if(IsKeyDown(KEY_LEFT)||IsKeyDown(KEY_A))  g_camTarget.x-=panSpd*dt;
    if(IsKeyDown(KEY_DOWN)||IsKeyDown(KEY_S))  g_camTarget.y+=panSpd*dt;
    if(IsKeyDown(KEY_UP)||IsKeyDown(KEY_W))    g_camTarget.y-=panSpd*dt;
    // Zoom (:2484-2489)
    float wheel=GetMouseWheelMove();
    if(wheel!=0){
        g_zoomTarget+=wheel*0.1f;
        g_zoomTarget=std::max(0.5f,std::min(2.f,g_zoomTarget));
    }
    // Clamp target (:2490-2494)
    float hw=(SCREEN_W*0.5f)/g_battle.cam.zoom;
    float hh=((SCREEN_H-HUD_H-TOPBAR_H)*0.5f)/g_battle.cam.zoom;
    g_camTarget.x=std::max(hw,std::min((float)BATTLE_W-hw,g_camTarget.x));
    g_camTarget.y=std::max(hh,std::min((float)BATTLE_H-hh,g_camTarget.y));
    // Smooth lerp (:2495-2500)
    g_battle.cam.target.x+=(g_camTarget.x-g_battle.cam.target.x)*8.f*dt;
    g_battle.cam.target.y+=(g_camTarget.y-g_battle.cam.target.y)*8.f*dt;
    g_battle.camZoom+=(g_zoomTarget-g_battle.camZoom)*6.f*dt;
    g_battle.cam.zoom=g_battle.camZoom;
    g_battle.cam.offset={(float)SCREEN_W*0.5f,(float)(SCREEN_H-HUD_H+TOPBAR_H)*0.5f};
}
