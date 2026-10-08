// src/sprite.h — Sprites: generación procedural + hojas externas (CC0)
#pragma once
#include "config.h"

// Firmas reales (:216, :262, :305, :349, :356)
Image     makeInfantrySprite(unsigned char r,unsigned char g,unsigned char b,int weaponHint);
Image     makeCavalrySprite (unsigned char r,unsigned char g,unsigned char b);
Image     makeRangedSprite  (unsigned char r,unsigned char g,unsigned char b,bool crossbow);
Texture2D imageToTex(Image img);
void      rebuildTexture(int idx);   // envuelve la lambda makeImg (:356-374)

// Hojas externas: assets/sprites/*.png (Kenney "Medieval RTS", CC0).
// Pose3/4 fija por unidad: el ángulo de dibujo solo elige espejo E/W.
// Si faltan los archivos, todo el juego sigue con los sprites procedurales.
bool      loadUnitSheets();           // idempotente; false si faltan assets
void      unloadUnitSheets();
bool      unitSheetsActive();
Texture2D unitTexture(int typeIdx,int team); // 0=aliado(azul),1=enemigo(rojo)

// Hojas isométricas bakeadas: assets/units/*.png (KayKit CC0, tools/bake_sprites).
// 8 unidades x 4 animaciones x 2 equipos, 8 direcciones por ángulo de facing.
// Cadena de reserva: iso -> Kenney -> procedural (faltan assets).
enum UnitAnim { UA_IDLE=0, UA_WALK, UA_ATTACK, UA_DEAD, UA_COUNT };
void      drawUnit(int typeIdx,int team,Vector2 pos,float angleDeg,
                   float scale,bool bright,bool shadow,
                   int anim=UA_IDLE,float animTime=0.f);
// Dibuja el frame de muerte (edad en segundos); false si no hay hojas iso
bool      drawUnitDeath(int typeIdx,int team,Vector2 pos,float angleDeg,
                        float scale,float age,float alpha);
// Altura en px de la cabeza sobre el punto de apoyo (para barras de HP)
float     unitHeadOffset(int typeIdx,float scale);
