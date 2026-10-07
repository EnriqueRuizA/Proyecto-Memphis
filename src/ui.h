// src/ui.h — Helpers de interfaz
#pragma once

#include "config.h"
#include "util.h"
#include <string>
#include <cstdio>

// Nombres reales del código (:54-61)
inline void DrawTextRaw(const char* text,int x,int y,int fontSize,Color col){ ::DrawText(text,x,y,fontSize,col); }
inline int  MeasureTextRaw(const char* text,int fontSize){ return ::MeasureText(text,fontSize); }
inline void DrawTextUI(const char* text,int x,int y,int fontSize,Color col){
    DrawTextRaw(text,x,y,uiFS(fontSize),col);
}
inline int  MeasureTextUI(const char* text,int fontSize){
    return MeasureTextRaw(text,uiFS(fontSize));
}

// CRÍTICO: el código llama a DrawText/MeasureText en ~1.000 sitios (:63-64).
#ifndef DrawText
#define DrawText    DrawTextUI
#endif
#ifndef MeasureText
#define MeasureText MeasureTextUI
#endif

// Firmas reales (declaraciones; definiciones en ui.cpp)
bool drawButton(Rectangle r,const char* lbl,Vector2 m,
                Color cn={40,55,40,255},Color ch={70,110,60,255});            // (:872)
bool drawSmBtn(Rectangle r,const char* lbl,Vector2 m,
               Color cn={30,40,30,255},Color ch={55,90,45,255});              // (:888)
int drawIntSlider(Rectangle r,int val,int mn,int mx,
                  const char* label,Vector2 mouse,Color fill={80,160,80,200}); // (:904)
float drawFloatSlider(Rectangle r,float val,float mn,float mx,
                      const char* label,const char* fmt,Vector2 mouse,
                      Color fill={80,160,80,200});                             // (:924)
void drawSoldierSprite(Texture2D tex,Vector2 pos,float angleDeg,float scale,Color tint,bool drawShadow); // (:944)
void drawHealthBar(Vector2 pos,float hp,float maxHp,float w,float scl=1.f);    // (:953)
void drawWrappedText(const char* text,int x,int y,int maxWidth,int fontSize,Color col); // (:963)
