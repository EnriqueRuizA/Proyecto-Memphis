// src/mapart.h — Arte procedural del mapa de campana (Fase C)
// Solo visual: la logica de provincias/ciudades no se toca.
#pragma once
#include "config.h"

struct City;

// Nivel de ciudad = 1 + edificios construidos (cap 16 = fortaleza)
int cityBuildLevel(const City& city);

void drawCampaignSea(float time); // mar animado de fondo
// Continente tipo Risk: celdas Voronoi por provincia (sembradas por center,
// recortadas al casco continental). Rellena, fronteras y costa.
void drawCampaignContinent(int campaignId);
// Hit-test polygonal sobre la celda (fallback circular si no hay celda)
bool pointInProvinceCell(int idx, Vector2 pt);
// Traza el contorno de la celda (seleccion / YOU / hover)
void drawProvinceCellOutline(int idx, Color col, float thick);
// Blob irregular deterministico por provincia (semilla=idx) — estilo islas
void drawProvinceBlob(int idx, Vector2 center, float rad,
                      Color fill, Color border, Color ownerRing);
// Decoracion de terreno dentro del blob (montanas/bosque/llanura/ola)
void drawTerrainDecor(int idx, TerrainType t, Vector2 center, float rad);
// Mini-ciudad acumulativa: L1-3 aldeas de paja, L4-7 casas+mercado,
// L8-11 keep+torres, L12-16 muralla+gate+bandera de faccion
void drawMiniCity(Vector2 pos, int level, FactionId owner, float time);
