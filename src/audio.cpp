// src/audio.cpp — Audio: pools de aliases por SFX + música streaming con crossfade
//
// Sin PlaySoundMulti en este build de raylib → cada SFX tiene N aliases
// (LoadSoundAlias comparte los datos de muestreo pero cada uno tiene su
// propio AudioBuffer, volumen y pitch) y se emiten en round-robin.
// Cooldown por id para evitar avalanchas de sonidos en combate denso.
#include "audio.h"
#include "config.h"
#include "raylib.h"
#include <cstdio>

// ── SFX ─────────────────────────────────────────────────────────────────────
static const int   SFX_POOL    = 4;     // aliases simultáneos por SFX
static const float SFX_MIN_GAP = 0.05f; // s mínimo entre emisiones del mismo id

static const char* SFX_PATH[SFX_COUNT]={
    "assets/audio/ui_click.ogg",       // SFX_UI_CLICK
    "assets/audio/ui_confirm.ogg",     // SFX_UI_CONFIRM
    "assets/audio/ui_error.ogg",       // SFX_UI_ERROR
    "assets/audio/ui_hover.ogg",       // SFX_SELECT (variante de click suave)
    "assets/audio/ui_confirm.ogg",     // SFX_ORDER_MOVE
    "assets/audio/swing1.wav",         // SFX_ORDER_ATTACK (whoosh)
    "assets/audio/hit_metal1.ogg",     // SFX_HIT_METAL0
    "assets/audio/hit_metal2.ogg",     // SFX_HIT_METAL1
    "assets/audio/hit_metal3.ogg",     // SFX_HIT_METAL2
    "assets/audio/hit_flesh.ogg",      // SFX_HIT_FLESH
    "assets/audio/death.ogg",          // SFX_DEATH
    "assets/audio/arrow_shoot.wav",    // SFX_ARROW_SHOOT
    "assets/audio/coin.ogg",           // SFX_COIN
};

static bool   s_ready=false;
static Sound  s_base[SFX_COUNT];                 // mantiene vivos los datos
static Sound  s_alias[SFX_COUNT][SFX_POOL];
static bool   s_ok[SFX_COUNT];
static int    s_rr[SFX_COUNT];
static double s_lastPlay[SFX_COUNT];

// ── MÚSICA ──────────────────────────────────────────────────────────────────
enum { MUS_MENU=0, MUS_BATTLE, MUS_VICTORY, MUS_COUNT };

static const char* MUS_PATH[MUS_COUNT]={
    "assets/audio/music_menu.wav",     // Epic March Loop (loop puro)
    "assets/audio/music_battle.mp3",   // Medieval: Battle
    "assets/audio/music_victory.mp3",  // Medieval: Victory Theme
};

static Music  s_mus[MUS_COUNT];
static bool   s_musOk[MUS_COUNT];
static int    s_cur  = -1;  // pista en fade-in / sonando
static int    s_out  = -1;  // pista en fade-out
static float  s_inVol = 0.f;
static float  s_outVol= 0.f;
static int    s_want  = -2; // último track pedido (init: fuerza arranque)
static float  s_musicVol=0.7f;
static GameState s_lastState=(GameState)-1;

// ── helpers ─────────────────────────────────────────────────────────────────
static int trackForState(GameState st){
    switch(st){
        case STATE_BATTLE:
        case STATE_BATTLE_RESULT: return MUS_BATTLE;
        case STATE_VICTORY:       return MUS_VICTORY;
        case STATE_DEFEAT:        return -1;   // derrota: silencio
        default:                  return MUS_MENU;
    }
}

static void startTrack(int want){
    if(want==s_cur) return;
    if(s_cur>=0){
        if(s_out>=0) StopMusicStream(s_mus[s_out]); // pisa el fade anterior
        s_out=s_cur; s_outVol=s_inVol;
        if(s_outVol<=0.f){ StopMusicStream(s_mus[s_out]); s_out=-1; }
    }
    s_cur=want; s_inVol=0.f;
    if(want>=0&&s_musOk[want]){
        PlayMusicStream(s_mus[want]);
        SetMusicVolume(s_mus[want],0.f);
    }
}

// ── API ─────────────────────────────────────────────────────────────────────
void initAudio(){
    InitAudioDevice();
    if(!IsAudioDeviceReady()){ TraceLog(LOG_WARNING,"AUDIO: device not ready, running silent"); return; }
    s_ready=true;

    for(int i=0;i<SFX_COUNT;i++){
        s_ok[i]=false; s_rr[i]=0; s_lastPlay[i]=0.0;
        s_base[i]=LoadSound(SFX_PATH[i]);
        if(!IsSoundValid(s_base[i])){
            TraceLog(LOG_WARNING,"AUDIO: failed to load %s",SFX_PATH[i]);
            continue;
        }
        for(int a=0;a<SFX_POOL;a++){
            s_alias[i][a]=LoadSoundAlias(s_base[i]);
            if(!IsSoundValid(s_alias[i][a])){ TraceLog(LOG_WARNING,"AUDIO: alias failed for %s",SFX_PATH[i]); break; }
            if(a==SFX_POOL-1) s_ok[i]=true;
        }
    }

    for(int m=0;m<MUS_COUNT;m++){
        s_musOk[m]=false;
        s_mus[m]=LoadMusicStream(MUS_PATH[m]);
        if(!IsMusicValid(s_mus[m])){ TraceLog(LOG_WARNING,"AUDIO: failed to load %s",MUS_PATH[m]); continue; }
        s_musOk[m]=true; // LoadMusicStream: looping=true por defecto
    }

    SetMasterVolume(g_settings.masterVolume);
    s_musicVol=g_settings.musicVolume;
    s_lastState=g_state;
    s_want=-2; // fuerza startTrack en el primer update
    TraceLog(LOG_INFO,"AUDIO: ready (%d sfx, %d music)",SFX_COUNT,MUS_COUNT);
}

void shutdownAudio(){
    if(!s_ready) return;
    s_ready=false;
    if(s_cur>=0&&s_musOk[s_cur]) StopMusicStream(s_mus[s_cur]);
    if(s_out>=0&&s_musOk[s_out]) StopMusicStream(s_mus[s_out]);
    for(int m=0;m<MUS_COUNT;m++) if(s_musOk[m]) UnloadMusicStream(s_mus[m]);
    for(int i=0;i<SFX_COUNT;i++){
        if(!s_ok[i]) continue;
        for(int a=0;a<SFX_POOL;a++) if(IsSoundValid(s_alias[i][a])) UnloadSoundAlias(s_alias[i][a]);
        UnloadSound(s_base[i]);
    }
    CloseAudioDevice();
}

void playSfx(int id,float vol,float pitch){
    if(!s_ready||id<0||id>=SFX_COUNT||!s_ok[id]) return;
    double now=GetTime();
    if(now-s_lastPlay[id]<(double)SFX_MIN_GAP) return;
    s_lastPlay[id]=now;
    Sound s=s_alias[id][s_rr[id]];
    s_rr[id]=(s_rr[id]+1)%SFX_POOL;
    float jit=0.94f+(float)GetRandomValue(0,120)/1000.f; // 0.94..1.06
    SetSoundVolume(s,vol);
    SetSoundPitch(s,pitch*jit);
    PlaySound(s);
}

void playSfxVar(int first,int count,float vol,float pitch){
    if(count<1) return;
    playSfx(first+GetRandomValue(0,count-1),vol,pitch);
}

void previewAudioVolumes(float master,float music){
    if(!s_ready) return;
    SetMasterVolume(master);
    s_musicVol=music;
    if(s_cur>=0&&s_musOk[s_cur]) SetMusicVolume(s_mus[s_cur],s_musicVol*s_inVol);
    if(s_out>=0&&s_musOk[s_out]) SetMusicVolume(s_mus[s_out],s_musicVol*s_outVol);
}

void updateAudio(float dt){
    if(!s_ready) return;

    // Al cambiar de pantalla: reaplicar volúmenes guardados (deshace previews
    // de Settings no guardados) y decidir la pista según el estado.
    if(g_state!=s_lastState){
        s_lastState=g_state;
        SetMasterVolume(g_settings.masterVolume);
        s_musicVol=g_settings.musicVolume;
    }

    int want=trackForState(g_state);
    if(want!=s_want){ s_want=want; startTrack(want); }

    float rate=dt*0.9f; // ~1.1 s de crossfade
    if(s_cur>=0&&s_inVol<1.f){
        s_inVol+=rate; if(s_inVol>1.f) s_inVol=1.f;
        if(s_musOk[s_cur]) SetMusicVolume(s_mus[s_cur],s_musicVol*s_inVol);
    }
    if(s_out>=0){
        s_outVol-=rate;
        if(s_outVol<=0.f){ StopMusicStream(s_mus[s_out]); s_out=-1; }
        else if(s_musOk[s_out]) SetMusicVolume(s_mus[s_out],s_musicVol*s_outVol);
    }
    if(s_cur>=0&&s_musOk[s_cur]&&IsMusicStreamPlaying(s_mus[s_cur])) UpdateMusicStream(s_mus[s_cur]);
    if(s_out >=0&&s_musOk[s_out] &&IsMusicStreamPlaying(s_mus[s_out])) UpdateMusicStream(s_mus[s_out]);
}
