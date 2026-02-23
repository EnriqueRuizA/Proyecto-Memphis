// MEDIEVAL CONQUEST — Definiciones de estado global
#include "game_globals.h"

int SCREEN_W = 1280;
int SCREEN_H = 768;

std::vector<UnitTypeDef> g_unitTypes;
std::vector<Texture2D>   g_playerTextures;
std::vector<Texture2D>   g_enemyTextures;

CampaignState g_campaign;
BattleState   g_battle;
BattleResult  g_lastResult;
PreBattleState g_preBattle;

GameState g_state = STATE_MAIN_MENU;
bool      g_quitRequested = false;
bool      g_hasSave = false;
float     g_menuTime = 0.f;

bool g_quickBattle = false;
QuickBattleSetup g_quickSetup;

int       g_editTypeIdx = 0;
bool      g_editorDropdownOpen = false;
bool      g_editorPreviewDirty = true;
Texture2D g_editorPreviewTex = {0};
float     g_editorPreviewAngle = 0.f;
Texture2D g_editorPrevTex = {0};
