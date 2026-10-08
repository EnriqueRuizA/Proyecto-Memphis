// src/audio.h — Audio: SFX + música streaming (Fase 3)
#pragma once

// SFX ids (variantes contiguas: use playSfxVar(first,count) para elegir al azar)
enum SfxId {
    SFX_UI_CLICK=0,
    SFX_UI_CONFIRM,
    SFX_UI_ERROR,
    SFX_SELECT,
    SFX_ORDER_MOVE,
    SFX_ORDER_ATTACK,
    SFX_HIT_METAL0, SFX_HIT_METAL1, SFX_HIT_METAL2,   // variantes
    SFX_HIT_FLESH,
    SFX_DEATH,
    SFX_ARROW_SHOOT,
    SFX_COIN,
    SFX_COUNT
};

void initAudio();       // tras InitWindow(); no-op si el dispositivo falla
void shutdownAudio();   // antes de CloseWindow()
void updateAudio(float dt); // una vez por frame: streams + crossfade + volúmenes
void playSfx(int id,float vol=1.f,float pitch=1.f);
void playSfxVar(int first,int count,float vol=1.f,float pitch=1.f);
void previewAudioVolumes(float master,float music); // previsualización en Settings
