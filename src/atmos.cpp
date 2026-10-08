// src/atmos.cpp — Atmósfera de batalla: viñeta radial, sombras de nube
// viajeras y motas de polvo flotante con deriva de viento.
#include "atmos.h"
#include "config.h"
#include <cmath>

// ── texturas (se generan una vez) ───────────────────────────────────────────
static Texture2D s_softMask = {};   // máscara radial suave (blanca, centro sólido)
static Texture2D s_vignette = {};   // radial: centro transparente, bordes oscuros
static bool s_ready = false;

// ── nubes: sombras oscuras elípticas que cruzan el campo ───────────────────
static const int CLOUD_N = 4;
struct Cloud { float x, y, rx, ry, vx, a; };
static Cloud s_clouds[CLOUD_N];

// ── motas: polvo cálido flotante con viento ─────────────────────────────────
static const int MOTE_N = 48;
struct Mote { Vector2 pos, vel; float life, maxLife, size, a0; bool active; };
static Mote s_motes[MOTE_N];
static int s_moteNext = 0;
static float s_moteTimer = 0.f;

static void spawnMote(){
    Mote& m = s_motes[s_moteNext];
    s_moteNext = (s_moteNext + 1) % MOTE_N;
    m.pos = { (float)GetRandomValue(0, BATTLE_W), (float)GetRandomValue(0, BATTLE_H) };
    m.vel = { (float)GetRandomValue(-90, 40) / 10.f, (float)GetRandomValue(-20, 60) / 10.f };
    m.maxLife = (float)GetRandomValue(40, 70) / 10.f;
    m.life = m.maxLife;
    m.size = (float)GetRandomValue(35, 55) / 10.f;
    m.a0 = (float)GetRandomValue(110, 160);
    m.active = true;
}

void atmosInit(){
    if(s_ready) return;

    // Máscara suave: blanco sólido en el centro -> transparente en el borde
    Image img = GenImageGradientRadial(128, 128, 0.9f,
                                       Color{255,255,255,255}, Color{255,255,255,0});
    s_softMask = LoadTextureFromImage(img);
    UnloadImage(img);

    // Viñeta: centro transparente -> borde oscuro (densidad baja = caída al borde)
    img = GenImageGradientRadial(256, 256, 0.55f,
                                 Color{0,0,0,0}, Color{0,0,0,150});
    s_vignette = LoadTextureFromImage(img);
    UnloadImage(img);

    s_ready = true;
    atmosReset();
}

void atmosShutdown(){
    if(!s_ready) return;
    UnloadTexture(s_softMask);
    UnloadTexture(s_vignette);
    s_ready = false;
}

void atmosReset(){
    for(int i=0;i<CLOUD_N;i++){
        s_clouds[i].x  = (float)GetRandomValue(-200, BATTLE_W);
        s_clouds[i].y  = (float)GetRandomValue(0, BATTLE_H);
        s_clouds[i].rx = (float)GetRandomValue(180, 420);
        s_clouds[i].ry = (float)GetRandomValue(110, 260);
        s_clouds[i].vx = (float)GetRandomValue(60, 160) / 10.f;
        s_clouds[i].a  = (float)GetRandomValue(30, 50);
    }
    for(auto& m : s_motes) m.active = false;
    s_moteTimer = 0.f;
}

void atmosUpdate(float dt){
    if(dt <= 0.f) return;

    for(auto& c : s_clouds){
        c.x += c.vx * dt;
        if(c.x - c.rx > BATTLE_W){ c.x = -c.rx; c.y = (float)GetRandomValue(0, BATTLE_H); }
    }

    s_moteTimer -= dt;
    if(s_moteTimer <= 0.f){
        s_moteTimer = 0.3f;
        spawnMote();
    }
    for(auto& m : s_motes){
        if(!m.active) continue;
        m.life -= dt;
        if(m.life <= 0.f){ m.active = false; continue; }
        m.pos.x += m.vel.x * dt;
        m.pos.y += m.vel.y * dt;
        m.vel.y += 2.f * dt;   // asiento suave
        m.vel.x *= (1.f - 0.2f * dt);
    }
}

void atmosDrawWorld(){
    if(!s_ready) return;

    // Sombra de nube (encima de las unidades: es una sombra)
    for(auto& c : s_clouds){
        DrawTexturePro(s_softMask,
            Rectangle{0,0,(float)s_softMask.width,(float)s_softMask.height},
            Rectangle{c.x-c.rx, c.y-c.ry, c.rx*2.f, c.ry*2.f},
            Vector2{0,0}, 0.f, Color{10,14,8,(unsigned char)c.a});
    }

    // Motas de polvo flotante
    for(auto& m : s_motes){
        if(!m.active) continue;
        float t = m.life / m.maxLife;             // 1 -> 0
        float fade = t < 0.5f ? t*2.f : (1.f-t)*2.f; // entra/sale suave
        int a = (int)(m.a0 * fade);
        if(a <= 0) continue;
        DrawCircleV(m.pos, m.size, Color{236,226,192,(unsigned char)a});
    }
}

void atmosDrawScreen(int w, int h){
    if(!s_ready) return;
    DrawTexturePro(s_vignette,
        Rectangle{0,0,(float)s_vignette.width,(float)s_vignette.height},
        Rectangle{0,0,(float)w,(float)h},
        Vector2{0,0}, 0.f, WHITE);
}
