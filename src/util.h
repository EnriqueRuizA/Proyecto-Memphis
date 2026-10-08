// src/util.h — Funciones utilitarias (nombres reales del código)
#pragma once

#include "config.h"
#include <cstdlib>
#include <cmath>
#include <random>

// --- RNG ACTUAL (rts_game.cpp:750-752). NO sustituir por frandU()/GetTime(). ---
extern std::mt19937 g_rng;                                   // definido en util.cpp
inline float frandMT(){ return std::uniform_real_distribution<float>(0.f,1.f)(g_rng); }
inline int   randIntMT(int lo,int hi){ return std::uniform_int_distribution<int>(lo,hi)(g_rng); }

// --- (:69-92) ---
inline constexpr unsigned char clampU8(int v){ return (unsigned char)(v<0?0:(v>255?255:v)); }
inline float vdist(Vector2 a,Vector2 b){
    float dx=a.x-b.x,dy=a.y-b.y; return sqrtf(dx*dx+dy*dy);
}
inline Vector2 vnorm(Vector2 v){
    float l=sqrtf(v.x*v.x+v.y*v.y);
    if(l<0.0001f) return {0.f,0.f};
    return {v.x/l,v.y/l};
}
inline float dirToAngle(Vector2 d){ return atan2f(-d.y,d.x)*RAD2DEG; }
inline float lerpAngle(float c,float t,float s){
    // s=rate*dt: sin clamp, un frame largo (dt>1/rate) rebasaba el objetivo
    // y con s>2 oscilaba divergente (soldados girando como peonzas)
    if(s<0.f) s=0.f;
    if(s>1.f) s=1.f;
    float d=t-c;
    while(d>180.f){d-=360.f;}
    while(d<-180.f){d+=360.f;}
    return c+d*s;
}
// Giro con velocidad limitada (grados/frame): el objetivo puede oscilar
// mucho (ruido de formacion), pero el soldado solo gira a maxStep por frame
inline float turnAngle(float c,float t,float maxStep){
    float d=t-c;
    while(d>180.f){d-=360.f;}
    while(d<-180.f){d+=360.f;}
    if(d>maxStep) d=maxStep;
    if(d<-maxStep) d=-maxStep;
    return c+d;
}
inline float frand(){ return (float)rand()/(float)RAND_MAX; }
inline bool ptInRect(Vector2 p,Rectangle r){
    return p.x>=r.x&&p.x<=r.x+r.width&&p.y>=r.y&&p.y<=r.y+r.height;
}
inline Vector2 v2add(Vector2 a,Vector2 b){return {a.x+b.x,a.y+b.y};}
inline Vector2 v2sub(Vector2 a,Vector2 b){return {a.x-b.x,a.y-b.y};}
inline Vector2 v2scale(Vector2 a,float s){return {a.x*s,a.y*s};}
inline float   v2dot(Vector2 a,Vector2 b){return a.x*b.x+a.y*b.y;}
inline float   v2len(Vector2 a){return sqrtf(a.x*a.x+a.y*a.y);}

// --- (:777-781) ---
template<class T>
inline bool validIdx(int i, const std::vector<T>& v){ return i>=0 && i<(int)v.size(); }
