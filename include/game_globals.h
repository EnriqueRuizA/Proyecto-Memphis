// MEDIEVAL CONQUEST — Declaraciones de estado global
#ifndef GAME_GLOBALS_H
#define GAME_GLOBALS_H

#include "game_types.h"

extern std::vector<UnitTypeDef> g_unitTypes;
extern std::vector<Texture2D>   g_playerTextures;
extern std::vector<Texture2D>   g_enemyTextures;

extern CampaignState g_campaign;
extern BattleState   g_battle;
extern BattleResult  g_lastResult;
extern PreBattleState g_preBattle;

extern GameState g_state;
extern bool      g_quitRequested;
extern bool      g_hasSave;
extern float     g_menuTime;

extern bool g_quickBattle;
extern QuickBattleSetup g_quickSetup;

extern int       g_editTypeIdx;
extern bool      g_editorDropdownOpen;
extern bool      g_editorPreviewDirty;
extern Texture2D g_editorPreviewTex;
extern float     g_editorPreviewAngle;
extern Texture2D g_editorPrevTex;

#endif
