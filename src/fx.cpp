// src/fx.cpp — VFX: pool estático de partículas (chispas, sangre sutil, polvo)
#include "fx.h"
#include "config.h"
#include "elements.h"
#include "combat.h"
#include <cmath>

static const int FX_MAX = 512;

struct FxParticle {
    Vector2 pos, vel;
    float   life, maxLife;
    float   grav;      // px/s^2 (+ = cae, - = sube)
    float   drag;      // 1/s
    float   size0, size1;
    unsigned char r, g, b;
    unsigned char a0;  // alpha máxima; actual = a0*(life/maxLife)
    bool    active;
};

static FxParticle s_p[FX_MAX];
static int s_next = 0;

static void spawn(Vector2 pos, Vector2 vel, float life, float grav, float drag,
                  int r, int g, int b, int a0, float size0, float size1){
    FxParticle& p = s_p[s_next];
    s_next = (s_next + 1) % FX_MAX;
    p.pos = pos; p.vel = vel;
    p.life = life; p.maxLife = life;
    p.grav = grav; p.drag = drag;
    p.r = (unsigned char)r; p.g = (unsigned char)g; p.b = (unsigned char)b;
    p.a0 = (unsigned char)a0;
    p.size0 = size0; p.size1 = size1;
    p.active = true;
}

void fxClear(){
    for(auto& p : s_p) p.active = false;
    s_next = 0;
}

void fxSpawnSparks(Vector2 pos, int n){
    for(int i=0;i<n;i++){
        float a = (float)GetRandomValue(0,6283)/100.f;
        float sp = (float)GetRandomValue(70,190);
        Vector2 v = { cosf(a)*sp, sinf(a)*sp };
        bool hot = GetRandomValue(0,1)!=0;
        // amarillo brillante -> naranja, vida corta, caen y se apagan
        spawn(pos, v, (float)GetRandomValue(22,45)/100.f, 260.f, 2.5f,
              255, hot?225:205, hot?120:50, 240, 3.2f, 0.8f);
    }
}

void fxSpawnBlood(Vector2 pos, int n){
    for(int i=0;i<n;i++){
        float a = (float)GetRandomValue(0,6283)/100.f;
        float sp = (float)GetRandomValue(30,110);
        Vector2 v = { cosf(a)*sp, sinf(a)*sp - 30.f };
        // rojo oscuro, alpha<=215 (255 max) para que sea sutil pero visible
        spawn(pos, v, (float)GetRandomValue(40,75)/100.f, 320.f, 1.6f,
              160, GetRandomValue(18,32), 16, GetRandomValue(165,215), 3.4f, 1.4f);
    }
}

void fxBattleDust(float dt){
    static float t = 0.f;
    t += dt;
    if(t < 0.12f) return;
    t = 0.f;
    for(int side=0; side<2; side++){
        std::vector<BattleUnit>& units = side ? g_battle.enemyUnits : g_battle.playerUnits;
        for(auto& bu : units){
            for(auto& s : bu.soldiers){
                if(!s.alive) continue;
                if(s.state!=SS_MOVING_SLOT && s.state!=SS_MOVING_TARGET) continue;
                if(GetRandomValue(0,39)!=0) continue;   // ~1 de 40 por muestreo
                Vector2 v = { (float)GetRandomValue(-14,14), (float)GetRandomValue(-22,-6) };
                spawn(s.pos, v, (float)GetRandomValue(55,95)/100.f, -18.f, 1.8f,
                      155, 145, 125, 75, 3.f, 7.5f);
            }
        }
    }
}

void fxUpdate(float dt){
    if(dt <= 0.f) return;
    for(auto& p : s_p){
        if(!p.active) continue;
        p.life -= dt;
        if(p.life <= 0.f){ p.active = false; continue; }
        p.vel.y += p.grav * dt;
        float d = 1.f - p.drag * dt;
        if(d < 0.f) d = 0.f;
        p.vel.x *= d; p.vel.y *= d;
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
    }
}

void fxDraw(){
    for(auto& p : s_p){
        if(!p.active) continue;
        float t = p.life / p.maxLife;          // 1 -> 0
        int a = (int)((float)p.a0 * t);
        if(a <= 0) continue;
        float sz = p.size1 + (p.size0 - p.size1) * t;
        // 8 segmentos: particulas pequenas, DrawCircleV (360) era caro con cientos activas
        DrawCircleSector(p.pos, sz, 0.f, 360.f, 8, Color{p.r, p.g, p.b, (unsigned char)a});
    }
}
