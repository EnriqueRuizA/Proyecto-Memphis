// src/sprite.h — Generación procedural de sprites
#pragma once
#include "config.h"

// Firmas reales (:216, :262, :305, :349, :356)
Image     makeInfantrySprite(unsigned char r,unsigned char g,unsigned char b,int weaponHint);
Image     makeCavalrySprite (unsigned char r,unsigned char g,unsigned char b);
Image     makeRangedSprite  (unsigned char r,unsigned char g,unsigned char b,bool crossbow);
Texture2D imageToTex(Image img);
void      rebuildTexture(int idx);   // envuelve la lambda makeImg (:356-374)
