#include "campaign.h"
#include "config.h"
#include "util.h"
#include "ui.h"
#include "elements.h"
#include "city.h"
#include "save.h"
#include "combat.h"
#include <vector>
#include <string>
#include <deque>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <random>
#include <functional>
#include <cassert>
#include <utility>

CampaignState  g_campaign;  // (:596)
PreBattleState g_preBattle; // (:676)

//  UI STATE GLOBALS (4.4, 4.5)
// ───────────────────────────────────────────────────────────────────────────
int g_deleteConfirmIdx  = -1;  // 4.4: garrison delete confirmation
int g_selectedProvince  = -1;  // 4.5: pulsing selected province

// ───────────────────────────────────────────────────────────────────────────
//  Fase D+: bando del jugador en el mapa
//  Territorio propio y de facciones aliadas => mismo color (el del jugador);
//  cada faccion enemiga conserva el suyo bien visible.
// ───────────────────────────────────────────────────────────────────────────
bool factionIsPlayerSide(FactionId f){
    if(f==FACTION_PLAYER) return true;
    if((int)f<0||(int)f>=FACTION_COUNT) return false;
    return g_campaign.allied[(int)f];
}
Color factionDisplayColor(FactionId f){
    if(factionIsPlayerSide(f)) return factionColors[FACTION_PLAYER];
    if((int)f<0||(int)f>=FACTION_COUNT) return factionColors[FACTION_NEUTRAL];
    return factionColors[(int)f];
}

// ───────────────────────────────────────────────────────────────────────────
//  GARRISON ARMIES PANEL (0.1: extracted from orphan code)
// ───────────────────────────────────────────────────────────────────────────
void drawGarrisonArmiesPanel(Province& prov, float gridY, float cellH, Vector2 mouse){
    // Draw a panel showing garrisoned armies for the given province
    // Currently shown inline in city management as part of the building grid;
    // this extracted function can be called for additional overlay
    float panelY=gridY+3*cellH+8;
    if(panelY+60>SCREEN_H-44) return;
    DrawRectangle(8,(int)panelY,(int)(SCREEN_W-16),54,{10,16,10,180});
    DrawRectangleLinesEx({8,panelY,(float)(SCREEN_W-16),54},1,{60,90,50,180});
    DrawText("Garrison:",(int)18,(int)(panelY+6),13,C_GOLD);
    if(prov.army.empty()){
        DrawText("(no garrison)",(int)100,(int)(panelY+8),12,C_SECONDARY);
    } else {
        int ax=100;
        for(int k=0;k<(int)prov.army.size()&&k<8;k++){
            auto [ti,cnt]=prov.army[k];
            if(ti>=unitTypeCount()) continue;
            const UnitTypeDef& td=g_unitTypes[ti];
            DrawRectangle(ax,(int)(panelY+6),14,14,{td.r,td.g,td.b,220});
            DrawText(TextFormat("%s x%d",td.name,cnt),ax+18,(int)(panelY+8),11,C_PARCHMENT);
            ax+=120+MeasureText(td.name,11);
        }
    }
    (void)mouse;
    (void)cellH;
}

// ───────────────────────────────────────────────────────────────────────────
//  CAMPAIGN MOVEMENT ARROWS (0.1: extracted function)
// ───────────────────────────────────────────────────────────────────────────
void drawCampaignMovementArrows(){
    // Draw arrows from player province to adjacent reachable provinces
    if(g_campaign.playerProvince<0||g_campaign.playerProvince>=(int)g_campaign.provinces.size()) return;
    Province& pp=g_campaign.provinces[g_campaign.playerProvince];
    for(int adj:pp.adjacent){
        if(adj<0||adj>=(int)g_campaign.provinces.size()) continue;
        Province& ap=g_campaign.provinces[adj];
        Vector2 dir=vnorm(v2sub(ap.center,pp.center));
        Vector2 mid={pp.center.x+dir.x*40.f,pp.center.y+dir.y*40.f};
        // Small arrow pointing toward adjacent
        Color arrowCol=factionIsPlayerSide(ap.owner)?C_ALLY:C_ENEMY_COL; // Fase D+
        DrawLineEx(mid,v2add(mid,v2scale(dir,24.f)),2.f,{arrowCol.r,arrowCol.g,arrowCol.b,120});
        // Arrowhead
        Vector2 tip=v2add(mid,v2scale(dir,24.f));
        Vector2 perp={-dir.y,dir.x};
        DrawTriangle(tip,
            v2sub(v2sub(tip,v2scale(dir,8.f)),v2scale(perp,5.f)),
            v2add(v2sub(tip,v2scale(dir,8.f)),v2scale(perp,5.f)),
            {arrowCol.r,arrowCol.g,arrowCol.b,100});
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  CAMPAIGN MAP GENERATION (multiple campaigns)
// ═══════════════════════════════════════════════════════════════════════════
struct ProvinceTemplate {
    const char* name; float nx,ny; TerrainType terrain; FactionId owner; bool hasCity;
    const char* cityName;
};

void generateCampaignMap(int campaignId){
    g_campaign.provinces.clear();
    g_campaign.campaignId = campaignId;
    int sw = SCREEN_W>0 ? SCREEN_W : 1280;
    int sh = SCREEN_H>0 ? SCREEN_H : 768;

    if(campaignId==0){
        // Campaign 0: "The Realm" — original 14 provinces
        static const ProvinceTemplate tpls[]={
            {"Aldenmoor",     0.18f,0.42f, TERRAIN_PLAIN,    FACTION_PLAYER,     true,  "Aldenmoor"},
            {"Greenvale",     0.30f,0.28f, TERRAIN_FOREST,   FACTION_PLAYER,     true,  "Greenvale"},
            {"Ironholt",      0.28f,0.62f, TERRAIN_MOUNTAIN, FACTION_NEUTRAL,    true,  "Ironholt"},
            {"Saltmere",      0.14f,0.72f, TERRAIN_COAST,    FACTION_NEUTRAL,    true,  "Saltmere"},
            {"Dustfield",     0.42f,0.50f, TERRAIN_PLAIN,    FACTION_NEUTRAL,    false, ""},
            {"Ashford",       0.52f,0.32f, TERRAIN_FOREST,   FACTION_AGGRESSIVE, true,  "Ashford"},
            {"Cragspire",     0.60f,0.18f, TERRAIN_MOUNTAIN, FACTION_AGGRESSIVE, true,  "Cragspire"},
            {"Redmarch",      0.65f,0.48f, TERRAIN_PLAIN,    FACTION_AGGRESSIVE, true,  "Redmarch"},
            {"Blackfen",      0.50f,0.68f, TERRAIN_FOREST,   FACTION_DEFENSIVE,  true,  "Blackfen"},
            {"Stonewall",     0.72f,0.65f, TERRAIN_MOUNTAIN, FACTION_DEFENSIVE,  true,  "Stonewall"},
            {"Harborgate",    0.35f,0.85f, TERRAIN_COAST,    FACTION_COMMERCIAL, true,  "Harborgate"},
            {"Goldenport",    0.55f,0.82f, TERRAIN_COAST,    FACTION_COMMERCIAL, true,  "Goldenport"},
            {"Thornwood",     0.78f,0.35f, TERRAIN_FOREST,   FACTION_AGGRESSIVE, false, ""},
            {"Highpass",      0.82f,0.52f, TERRAIN_MOUNTAIN, FACTION_DEFENSIVE,  false, ""},
        };
        static const int adjData[][2]={
            {0,1},{0,2},{0,3},{1,4},{1,5},{2,3},{2,4},{2,8},
            {3,10},{4,5},{4,7},{4,8},{5,6},{5,7},{6,7},{6,12},
            {7,8},{7,9},{8,10},{8,11},{9,11},{9,13},{10,11},{12,13}
        };
        int n=14;
        for(int i=0;i<n;i++){
            const ProvinceTemplate& tp=tpls[i];
            Province p{};
            strncpy(p.name,tp.name,sizeof(p.name)-1); p.name[sizeof(p.name)-1]='\0';
            p.nx=tp.nx; p.ny=tp.ny;
            p.center={tp.nx*(float)sw, tp.ny*(float)sh};
            p.terrain=tp.terrain; p.owner=tp.owner; p.hasCity=tp.hasCity;
        if(tp.hasCity){
            strncpy(p.city.name,tp.cityName,sizeof(p.city.name)-1); p.city.name[sizeof(p.city.name)-1]='\0';
            memset(p.city.built,0,sizeof(p.city.built));
            p.city.constructing=-1;
            p.city.constructTurns=0;
            p.city.defBonus=1.0f;
            // Player starter city has barracks + farm
            if(tp.owner==FACTION_PLAYER){
                p.city.built[BLD_FARM]=true;
                p.city.built[BLD_BARRACKS]=true;
            }
            // Enemy cities have some buildings
            if(tp.owner==FACTION_AGGRESSIVE||tp.owner==FACTION_DEFENSIVE){
                p.city.built[BLD_FARM]=true;
                p.city.built[BLD_BARRACKS]=true;
            }
            if(tp.owner==FACTION_COMMERCIAL){
                p.city.built[BLD_FARM]=true;
                p.city.built[BLD_MARKET]=true;
            }
        }
        p.army.clear();
        // Add enemy armies
        if(tp.owner==FACTION_AGGRESSIVE){
            p.army.push_back({0,40}); // Spear Levy x40
            p.army.push_back({1,30}); // Axe Militia x30
        } else if(tp.owner==FACTION_DEFENSIVE){
            p.army.push_back({0,50});
            p.army.push_back({4,20}); // Bow Levy
        } else if(tp.owner==FACTION_COMMERCIAL){
            p.army.push_back({0,30});
        }
        g_campaign.provinces.push_back(p);
        }
        for(auto& pr:g_campaign.provinces) pr.adjacent.clear();
        for(auto& ad:adjData){
            if(ad[0]<n&&ad[1]<n){
                g_campaign.provinces[ad[0]].adjacent.push_back(ad[1]);
                g_campaign.provinces[ad[1]].adjacent.push_back(ad[0]);
            }
        }
    } else if(campaignId==1){
        // Campaign 1: "Northern Isles" — 10 provinces, island layout
        static const ProvinceTemplate tpls[]={
            {"Frostholm",   0.22f,0.35f, TERRAIN_MOUNTAIN, FACTION_PLAYER,   true,  "Frostholm"},
            {"Icewind",     0.38f,0.22f, TERRAIN_PLAIN,    FACTION_PLAYER,   true,  "Icewind"},
            {"Northgate",   0.55f,0.28f, TERRAIN_FOREST,   FACTION_NEUTRAL,  true,  "Northgate"},
            {"Stormhaven",  0.72f,0.40f, TERRAIN_COAST,    FACTION_AGGRESSIVE, true, "Stormhaven"},
            {"Mistwood",    0.48f,0.55f, TERRAIN_FOREST,   FACTION_NEUTRAL,  false, ""},
            {"Cinderfell",  0.28f,0.62f, TERRAIN_PLAIN,    FACTION_AGGRESSIVE, true, "Cinderfell"},
            {"Saltmarsh",   0.15f,0.78f, TERRAIN_COAST,   FACTION_COMMERCIAL, true, "Saltmarsh"},
            {"Boulder",     0.62f,0.68f, TERRAIN_MOUNTAIN, FACTION_DEFENSIVE, true, "Boulder"},
            {"Grayvale",    0.80f,0.72f, TERRAIN_FOREST,   FACTION_DEFENSIVE, false, ""},
            {"Port Snow",   0.42f,0.82f, TERRAIN_COAST,    FACTION_NEUTRAL,  true,  "Port Snow"},
        };
        static const int adjData[][2]={
            {0,1},{0,4},{0,5},{1,2},{1,4},{2,3},{2,4},{2,7},{3,7},{4,5},{4,7},{4,9},{5,6},{5,9},{6,9},{7,8},{8,9}
        };
        int n=10;
        for(int i=0;i<n;i++){
            const ProvinceTemplate& tp=tpls[i];
            Province p{};
            strncpy(p.name,tp.name,sizeof(p.name)-1); p.name[sizeof(p.name)-1]='\0';
            p.nx=tp.nx; p.ny=tp.ny;
            p.center={tp.nx*(float)sw, tp.ny*(float)sh};
            p.terrain=tp.terrain; p.owner=tp.owner; p.hasCity=tp.hasCity;
            if(tp.hasCity){
                strncpy(p.city.name,tp.cityName,sizeof(p.city.name)-1); p.city.name[sizeof(p.city.name)-1]='\0';
                memset(p.city.built,0,sizeof(p.city.built));
                p.city.constructing=-1; p.city.constructTurns=0; p.city.defBonus=1.0f;
                if(tp.owner==FACTION_PLAYER){ p.city.built[BLD_FARM]=true; p.city.built[BLD_BARRACKS]=true; }
                if(tp.owner==FACTION_AGGRESSIVE||tp.owner==FACTION_DEFENSIVE){ p.city.built[BLD_FARM]=true; p.city.built[BLD_BARRACKS]=true; }
                if(tp.owner==FACTION_COMMERCIAL){ p.city.built[BLD_FARM]=true; p.city.built[BLD_MARKET]=true; }
            }
            p.army.clear();
            if(tp.owner==FACTION_AGGRESSIVE){ p.army.push_back({0,40}); p.army.push_back({1,30}); }
            else if(tp.owner==FACTION_DEFENSIVE){ p.army.push_back({0,50}); p.army.push_back({4,20}); }
            else if(tp.owner==FACTION_COMMERCIAL){ p.army.push_back({0,30}); }
            g_campaign.provinces.push_back(p);
        }
        for(auto& pr:g_campaign.provinces) pr.adjacent.clear();
        for(auto& ad:adjData){
            if(ad[0]<n&&ad[1]<n){
                g_campaign.provinces[ad[0]].adjacent.push_back(ad[1]);
                g_campaign.provinces[ad[1]].adjacent.push_back(ad[0]);
            }
        }
    } else if(campaignId==2){
        // Campaign 2: "Eastern March" — 12 provinces
        static const ProvinceTemplate tpls[]={
            {"Riverdale",   0.20f,0.45f, TERRAIN_PLAIN,    FACTION_PLAYER,   true,  "Riverdale"},
            {"Oakshire",    0.35f,0.32f, TERRAIN_FOREST,  FACTION_PLAYER,   true,  "Oakshire"},
            {"Eastfort",    0.52f,0.38f, TERRAIN_MOUNTAIN, FACTION_NEUTRAL,  true,  "Eastfort"},
            {"Seabridge",   0.68f,0.50f, TERRAIN_COAST,   FACTION_AGGRESSIVE, true, "Seabridge"},
            {"Wheatfield",  0.42f,0.58f, TERRAIN_PLAIN,   FACTION_NEUTRAL,  false, ""},
            {"Blackwood",   0.58f,0.65f, TERRAIN_FOREST,  FACTION_DEFENSIVE, true, "Blackwood"},
            {"Cragdale",    0.75f,0.72f, TERRAIN_MOUNTAIN, FACTION_DEFENSIVE, true, "Cragdale"},
            {"Sandport",    0.28f,0.78f, TERRAIN_COAST,   FACTION_COMMERCIAL, true, "Sandport"},
            {"Millbrook",   0.12f,0.55f, TERRAIN_PLAIN,   FACTION_NEUTRAL,  true,  "Millbrook"},
            {"Pinewatch",   0.82f,0.35f, TERRAIN_FOREST,  FACTION_AGGRESSIVE, false, ""},
            {"Ironford",    0.45f,0.82f, TERRAIN_PLAIN,   FACTION_NEUTRAL,  false, ""},
            {"Highcliff",   0.65f,0.22f, TERRAIN_MOUNTAIN, FACTION_AGGRESSIVE, true, "Highcliff"},
        };
        static const int adjData[][2]={
            {0,1},{0,4},{0,8},{1,2},{1,4},{2,3},{2,4},{2,9},{2,11},{3,6},{3,9},{4,5},{4,10},{5,6},{5,10},{6,9},{7,8},{7,10},{8,10}
        };
        int n=12;
        for(int i=0;i<n;i++){
            const ProvinceTemplate& tp=tpls[i];
            Province p{};
            strncpy(p.name,tp.name,sizeof(p.name)-1); p.name[sizeof(p.name)-1]='\0';
            p.nx=tp.nx; p.ny=tp.ny;
            p.center={tp.nx*(float)sw, tp.ny*(float)sh};
            p.terrain=tp.terrain; p.owner=tp.owner; p.hasCity=tp.hasCity;
            if(tp.hasCity){
                strncpy(p.city.name,tp.cityName,sizeof(p.city.name)-1); p.city.name[sizeof(p.city.name)-1]='\0';
                memset(p.city.built,0,sizeof(p.city.built));
                p.city.constructing=-1; p.city.constructTurns=0; p.city.defBonus=1.0f;
                if(tp.owner==FACTION_PLAYER){ p.city.built[BLD_FARM]=true; p.city.built[BLD_BARRACKS]=true; }
                if(tp.owner==FACTION_AGGRESSIVE||tp.owner==FACTION_DEFENSIVE){ p.city.built[BLD_FARM]=true; p.city.built[BLD_BARRACKS]=true; }
                if(tp.owner==FACTION_COMMERCIAL){ p.city.built[BLD_FARM]=true; p.city.built[BLD_MARKET]=true; }
            }
            p.army.clear();
            if(tp.owner==FACTION_AGGRESSIVE){ p.army.push_back({0,40}); p.army.push_back({1,30}); }
            else if(tp.owner==FACTION_DEFENSIVE){ p.army.push_back({0,50}); p.army.push_back({4,20}); }
            else if(tp.owner==FACTION_COMMERCIAL){ p.army.push_back({0,30}); }
            g_campaign.provinces.push_back(p);
        }
        for(auto& pr:g_campaign.provinces) pr.adjacent.clear();
        for(auto& ad:adjData){
            if(ad[0]<n&&ad[1]<n){
                g_campaign.provinces[ad[0]].adjacent.push_back(ad[1]);
                g_campaign.provinces[ad[1]].adjacent.push_back(ad[0]);
            }
        }
    }
}

void updateProvinceCenters(){
    for(int i=0;i<(int)g_campaign.provinces.size();i++){
        Province& p=g_campaign.provinces[i];
        p.center={p.nx*(float)SCREEN_W, p.ny*(float)SCREEN_H};
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  NEW CAMPAIGN INIT
// ═══════════════════════════════════════════════════════════════════════════
void newCampaign(int campaignId){
    if(campaignId<0||campaignId>=MAX_CAMPAIGNS) campaignId=0;
    g_campaign=CampaignState{};
    g_campaign.campaignId=campaignId;
    g_campaign.res.gold=500.f;
    g_campaign.res.food=200.f;
    g_campaign.res.wood=100.f;
    g_campaign.res.stone=50.f;
    g_campaign.res.iron=50.f;
    g_campaign.playerProvince=0;
    generateCampaignMap(campaignId);
    updateProvinceCenters();
    // Player starts with 3 spear levy groups
    g_campaign.playerArmy.push_back({0,60});
    g_campaign.playerArmy.push_back({0,60});
    g_campaign.playerArmy.push_back({4,40}); // Bow levy
    g_campaign.readyUnits=g_campaign.playerArmy;
    g_campaign.pendingBattleProvince=-1;
    g_campaign.turn=1;
}

// ═══════════════════════════════════════════════════════════════════════════
//  TURN PROCESSING (centralized)
// ═══════════════════════════════════════════════════════════════════════════
void processTurn(){
    Resources& r=g_campaign.res;

    // Advance recruitment queue
    for(auto& e:g_campaign.recruitQueue){
        e.turnsLeft--;
        if(e.turnsLeft<=0){
            g_campaign.readyUnits.push_back({e.typeIdx,e.typeIdx<(int)g_unitTypes.size()?g_unitTypes[e.typeIdx].soldierCount:40});
            battleLogAdd(TextFormat("[T%d] %s recruitment complete!", g_campaign.turn, g_unitTypes[e.typeIdx].name));
        }
    }
    // Remove completed recruitment
    g_campaign.recruitQueue.erase(
        std::remove_if(g_campaign.recruitQueue.begin(), g_campaign.recruitQueue.end(),
            [](const RecruitEntry& e){ return e.turnsLeft<=0; }),
        g_campaign.recruitQueue.end()
    );

    // Resource generation from ALL player cities
    for(int pi=0;pi<(int)g_campaign.provinces.size();pi++){
        Province& prov=g_campaign.provinces[pi];
        if(prov.owner!=FACTION_PLAYER||!prov.hasCity) continue;
        City& c=prov.city;
        for(int b=0;b<BLD_COUNT;b++){
            if(!c.built[b]) continue;
            r.gold+=bldGoldYield[b];
            r.food+=bldFoodYield[b];
            r.wood+=bldWoodYield[b];
            r.stone+=bldStoneYield[b];
            r.iron+=bldIronYield[b];
        }
        // Advance construction
        if(c.constructing>=0){
            c.constructTurns--;
            if(c.constructTurns<=0){
                c.built[c.constructing]=true;
                battleLogAdd(TextFormat("[T%d] %s: %s completed!", g_campaign.turn, prov.name, bldNames[c.constructing]));
                c.constructing=-1;
            }
        }
    }

    // Maintenance from all units
    for(auto [ti,cnt]:g_campaign.readyUnits){
        if(ti>=unitTypeCount()) continue;
        const UnitTypeDef& td=g_unitTypes[ti];
        r.gold-=td.maintGold*(cnt/(float)td.soldierCount);
        r.food-=td.maintFood*(cnt/(float)td.soldierCount);
    }

    // Clamp resources (don't go negative)
    r.gold=std::max(0.f,r.gold);
    r.food=std::max(0.f,r.food);
    r.wood=std::max(0.f,r.wood);
    r.stone=std::max(0.f,r.stone);
    r.iron=std::max(0.f,r.iron);

    // Enemy AI and actions
    float aggression=0.05f+0.02f*(float)g_campaign.turn/10.f;
    aggression=std::min(aggression,0.4f);
    static const float diffMult[3]={0.7f,1.0f,1.4f};
    aggression*=diffMult[std::max(0,std::min(2,g_settings.difficulty))];

    // Coalition detection
    std::vector<int> coalitionAttackers;
    for(int pi2=0;pi2<(int)g_campaign.provinces.size();pi2++){
        Province& ep2=g_campaign.provinces[pi2];
        if(ep2.owner==FACTION_NEUTRAL||ep2.owner==FACTION_PLAYER) continue;
        bool adjPlayer=false;
        for(int adj2:ep2.adjacent) if(g_campaign.provinces[adj2].owner==FACTION_PLAYER) adjPlayer=true;
        if(!adjPlayer) continue;
        for(int adj2:ep2.adjacent){
            Province& ep3=g_campaign.provinces[adj2];
            if(ep3.owner!=ep2.owner) continue;
            bool partnerAdjPlayer=false;
            for(int adj3:ep3.adjacent) if(g_campaign.provinces[adj3].owner==FACTION_PLAYER) partnerAdjPlayer=true;
            if(partnerAdjPlayer){
                coalitionAttackers.push_back(pi2);
                break;
            }
        }
    }

    for(int pi2=0;pi2<(int)g_campaign.provinces.size();pi2++){
        Province& ep=g_campaign.provinces[pi2];
        if(ep.owner==FACTION_NEUTRAL||ep.owner==FACTION_PLAYER) continue;

        float roll=frandMT();
        bool inCoalition=false;
        for(int ca:coalitionAttackers) if(ca==pi2){inCoalition=true;break;}
        float attackChance=inCoalition?aggression*2.f:aggression;

        if(roll<attackChance*0.4f){
            // Attack weak neighbor
            for(int adj:ep.adjacent){
                if(g_campaign.provinces[adj].owner==FACTION_PLAYER){
                    int eStr=0; for(auto [ti,c]:ep.army) eStr+=c;
                    int pStr=(int)g_campaign.readyUnits.size()*40;
                    if(eStr>(int)(pStr*1.2f)){
                        g_preBattle.provinceIdx=adj;
                        g_preBattle.isDefense=true;
                        g_preBattle.fogOfWar=false;
                        g_preBattle.estimatedEnemyStrength=eStr;
                        g_preBattle.include.clear();
                        g_preBattle.include.resize(g_campaign.readyUnits.size(),true);
                        for(int j=12;j<(int)g_preBattle.include.size();j++)
                            g_preBattle.include[j]=false;
                        g_campaign.provinces[adj].army=ep.army;
                        g_campaign.battlesLost=g_campaign.battlesLost;
                        // Don't return here - we'll handle battle after incrementing turn
                    }
                    break;
                }
            }
        } else if(roll<attackChance*0.4f+0.2f){
            // Reinforcement transfer
            for(int adj:ep.adjacent){
                Province& ap=g_campaign.provinces[adj];
                if(ap.owner!=ep.owner||ap.army.empty()) continue;
                int apStr=0; for(auto [ti,c]:ap.army) apStr+=c;
                int epStr=0; for(auto [ti,c]:ep.army) epStr+=c;
                if(apStr>epStr+20&&!ap.army.empty()){
                    auto& transferUnit=ap.army.front();
                    int transferAmt=transferUnit.second/4;
                    if(transferAmt>0){
                        bool found=false;
                        for(auto& eu:ep.army) if(eu.first==transferUnit.first){eu.second+=transferAmt;found=true;break;}
                        if(!found) ep.army.push_back({transferUnit.first,transferAmt});
                        transferUnit.second-=transferAmt;
                        if(transferUnit.second<=0) ap.army.erase(ap.army.begin());
                    }
                    break;
                }
            }
        }
    }

    // Check victory condition
    int totalProv=(int)g_campaign.provinces.size();
    int playerProv=0;
    for(auto& p:g_campaign.provinces) if(p.owner==FACTION_PLAYER) playerProv++;
    g_campaign.peakProvinces=std::max(g_campaign.peakProvinces,playerProv);
    if(playerProv>=(int)(totalProv*0.8f)){
        battleLogAdd("[Campaign] VICTORY! Empire established!");
    }
    if(playerProv==0){
        battleLogAdd("[Campaign] DEFEAT — all provinces lost!");
    }

    // Finally: Increment turn after all processing
    g_campaign.turn++;
}

// ═══════════════════════════════════════════════════════════════════════════

// (:690, :692-693) — Resource trading (externs en campaign.h)
const float TRADE_RATES[5]={1.f, 1.2f, 1.3f, 1.5f, 0.8f}; // gold, food, wood, stone, iron
int g_tradeTab=0; // 0-4 for each resource
int g_tradeAmount=10; // amount to buy/sell
