// src/save.h — Serialización binaria (formato actual: binario con SAVE_VERSION=5)
#pragma once

inline const char* CAMPAIGN_LAST_FILE = "campaign_last.dat";
inline const int   MAX_CAMPAIGNS      = 4;
inline const char* CAMPAIGN_NAMES[MAX_CAMPAIGNS] = { "The Realm", "Northern Isles", "Eastern March", "Great Continent" };

const char* getCampaignSavePath(int id);  // (:28-33)
void saveGame();   // (:4463) — SIN parámetros
bool loadGame();   // (:4530) — SIN parámetros
// Usa campaign_%d.dat / campaign_last.dat. fwrite/fread con SAVE_VERSION (:4467, :4541).
