#include "save.h"
#include "config.h"
#include "util.h"
#include "ui.h"
#include "campaign.h"
#include "elements.h"
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

const char* getCampaignSavePath(int id){
    static char buf[32];
    if(id<0) id=0; if(id>=MAX_CAMPAIGNS) id=MAX_CAMPAIGNS-1;
    sprintf(buf, "campaign_%d.dat", id);
    return buf;
}
//  SAVE / LOAD — Campaign persistence per campaign file
// ═══════════════════════════════════════════════════════════════════════════

void saveGame(){
    const char* path=getCampaignSavePath(g_campaign.campaignId);
    FILE* f=fopen(path,"wb");
    if(!f) return;
    fwrite(&SAVE_VERSION,sizeof(int),1,f);
    fwrite(&g_campaign.campaignId,sizeof(int),1,f);
    fwrite(&g_campaign.turn,sizeof(int),1,f);
    fwrite(&g_campaign.res,sizeof(Resources),1,f);
    fwrite(&g_campaign.playerProvince,sizeof(int),1,f);
    fwrite(&g_campaign.pendingBattleProvince,sizeof(int),1,f);
    fwrite(&g_campaign.pendingBattleIsDefense,sizeof(bool),1,f);
    fwrite(&g_campaign.viewedCity,sizeof(int),1,f);

    int np=(int)g_campaign.provinces.size();
    fwrite(&np,sizeof(int),1,f);
    for(auto& p:g_campaign.provinces){
        fwrite(p.name,1,32,f);
        fwrite(&p.center.x,sizeof(float),1,f);
        fwrite(&p.center.y,sizeof(float),1,f);
        fwrite(&p.nx,sizeof(float),1,f);
        fwrite(&p.ny,sizeof(float),1,f);
        int ter=(int)p.terrain; fwrite(&ter,sizeof(int),1,f);
        int own=(int)p.owner;   fwrite(&own,sizeof(int),1,f);
        fwrite(&p.hasCity,sizeof(bool),1,f);
        if(p.hasCity){
            fwrite(p.city.name,1,32,f);
            fwrite(p.city.built,sizeof(bool),BLD_COUNT,f);
            fwrite(&p.city.constructing,sizeof(int),1,f);
            fwrite(&p.city.constructTurns,sizeof(int),1,f);
            fwrite(&p.city.defBonus,sizeof(float),1,f);
        }
        int nadj=(int)p.adjacent.size();
        fwrite(&nadj,sizeof(int),1,f);
        for(int a : p.adjacent) fwrite(&a,sizeof(int),1,f);
        int narmy=(int)p.army.size();
        fwrite(&narmy,sizeof(int),1,f);
        for(auto& ap : p.army){ fwrite(&ap.first,sizeof(int),1,f); fwrite(&ap.second,sizeof(int),1,f); }
    }

    int nq=(int)g_campaign.recruitQueue.size();
    fwrite(&nq,sizeof(int),1,f);
    for(auto& e:g_campaign.recruitQueue){
        fwrite(&e.typeIdx,sizeof(int),1,f);
        fwrite(&e.turnsLeft,sizeof(int),1,f);
    }

    int na=(int)g_campaign.playerArmy.size();
    fwrite(&na,sizeof(int),1,f);
    for(auto& a:g_campaign.playerArmy){
        fwrite(&a.first,sizeof(int),1,f);
        fwrite(&a.second,sizeof(int),1,f);
    }

    int nr=(int)g_campaign.readyUnits.size();
    fwrite(&nr,sizeof(int),1,f);
    for(auto& r:g_campaign.readyUnits){
        fwrite(&r.first,sizeof(int),1,f);
        fwrite(&r.second,sizeof(int),1,f);
    }

    // Fase J: ejércitos de campo + reserva
    int nA=(int)g_campaign.armies.size();
    fwrite(&nA,sizeof(int),1,f);
    for(auto& a:g_campaign.armies){
        int id=a.id, prov=a.province, own=(int)a.owner;
        unsigned char mv=a.moved?1:0;
        fwrite(&id,sizeof(int),1,f);
        fwrite(&own,sizeof(int),1,f);
        fwrite(&prov,sizeof(int),1,f);
        fwrite(&mv,sizeof(unsigned char),1,f);
        int nu=(int)a.units.size();
        fwrite(&nu,sizeof(int),1,f);
        for(auto& u:a.units){
            fwrite(&u.first,sizeof(int),1,f);
            fwrite(&u.second,sizeof(int),1,f);
        }
    }
    int nR=(int)g_campaign.reserve.size();
    fwrite(&nR,sizeof(int),1,f);
    for(auto& r:g_campaign.reserve){
        fwrite(&r.first,sizeof(int),1,f);
        fwrite(&r.second,sizeof(int),1,f);
    }

    // Fase I: diplomacia — alianzas militares + pactos comerciales
    fwrite(g_campaign.allied,sizeof(bool),FACTION_COUNT,f);
    fwrite(g_campaign.tradePact,sizeof(bool),FACTION_COUNT,f);

    fclose(f);
    g_hasSave=true;
    // Remember last played campaign for Continue
    FILE* lf=fopen(CAMPAIGN_LAST_FILE,"wb");
    if(lf){ fwrite(&g_campaign.campaignId,sizeof(int),1,lf); fclose(lf); }
}

bool loadGame(){
    int loadId=0;
    FILE* lf=fopen(CAMPAIGN_LAST_FILE,"rb");
    if(lf&&fread(&loadId,sizeof(int),1,lf)==1){ fclose(lf); }
    else if(lf) fclose(lf);
    if(loadId<0||loadId>=MAX_CAMPAIGNS) loadId=0;

    const char* path=getCampaignSavePath(loadId);
    FILE* f=fopen(path,"rb");
    if(!f) return false;
    int ver=0;
    if(fread(&ver,sizeof(int),1,f)!=1||ver!=SAVE_VERSION){ fclose(f); return false; }

    if(fread(&g_campaign.campaignId,sizeof(int),1,f)!=1){ fclose(f); return false; }
    if(fread(&g_campaign.turn,sizeof(int),1,f)!=1){ fclose(f); return false; }
    if(fread(&g_campaign.res,sizeof(Resources),1,f)!=1){ fclose(f); return false; }
    if(fread(&g_campaign.playerProvince,sizeof(int),1,f)!=1){ fclose(f); return false; }
    if(fread(&g_campaign.pendingBattleProvince,sizeof(int),1,f)!=1){ fclose(f); return false; }
    if(fread(&g_campaign.pendingBattleIsDefense,sizeof(bool),1,f)!=1){ fclose(f); return false; }
    if(fread(&g_campaign.viewedCity,sizeof(int),1,f)!=1){ fclose(f); return false; }

    int np=0;
    if(fread(&np,sizeof(int),1,f)!=1||np<=0||np>256){ fclose(f); return false; }
    g_campaign.provinces.clear();
    g_campaign.provinces.resize(np);
    for(int i=0;i<np;i++){
        Province& p=g_campaign.provinces[i];
        if(fread(p.name,1,32,f)!=32){ fclose(f); return false; }
        if(fread(&p.center.x,sizeof(float),1,f)!=1){ fclose(f); return false; }
        if(fread(&p.center.y,sizeof(float),1,f)!=1){ fclose(f); return false; }
        if(fread(&p.nx,sizeof(float),1,f)!=1){ fclose(f); return false; }
        if(fread(&p.ny,sizeof(float),1,f)!=1){ fclose(f); return false; }
        int ter=0,own=0;
        if(fread(&ter,sizeof(int),1,f)!=1){ fclose(f); return false; }
        if(fread(&own,sizeof(int),1,f)!=1){ fclose(f); return false; }
        p.terrain=(TerrainType)ter;
        p.owner=(FactionId)own;
        if(fread(&p.hasCity,sizeof(bool),1,f)!=1){ fclose(f); return false; }
        if(p.hasCity){
            if(fread(p.city.name,1,32,f)!=32){ fclose(f); return false; }
            if(fread(p.city.built,sizeof(bool),BLD_COUNT,f)!=BLD_COUNT){ fclose(f); return false; }
            if(fread(&p.city.constructing,sizeof(int),1,f)!=1){ fclose(f); return false; }
            if(fread(&p.city.constructTurns,sizeof(int),1,f)!=1){ fclose(f); return false; }
            if(fread(&p.city.defBonus,sizeof(float),1,f)!=1){ fclose(f); return false; }
        }
        int nadj=0;
        if(fread(&nadj,sizeof(int),1,f)!=1||nadj<0||nadj>64){ fclose(f); return false; }
        p.adjacent.clear();
        for(int j=0;j<nadj;j++){ int a=0; if(fread(&a,sizeof(int),1,f)!=1){ fclose(f); return false; } p.adjacent.push_back(a); }
        int narmy=0;
        if(fread(&narmy,sizeof(int),1,f)!=1||narmy<0||narmy>128){ fclose(f); return false; }
        p.army.clear();
        for(int j=0;j<narmy;j++){
            int ti=0,cnt=0;
            if(fread(&ti,sizeof(int),1,f)!=1||fread(&cnt,sizeof(int),1,f)!=1){ fclose(f); return false; }
            p.army.push_back({ti,cnt});
        }
    }

    int nq=0;
    if(fread(&nq,sizeof(int),1,f)!=1||nq<0||nq>256){ fclose(f); return false; }
    g_campaign.recruitQueue.clear();
    for(int i=0;i<nq;i++){
        int ti=0,tl=0;
        if(fread(&ti,sizeof(int),1,f)!=1||fread(&tl,sizeof(int),1,f)!=1){ fclose(f); return false; }
        g_campaign.recruitQueue.push_back({ti,tl});
    }

    int na=0;
    if(fread(&na,sizeof(int),1,f)!=1||na<0||na>256){ fclose(f); return false; }
    g_campaign.playerArmy.clear();
    for(int i=0;i<na;i++){
        int ti=0,cnt=0;
        if(fread(&ti,sizeof(int),1,f)!=1||fread(&cnt,sizeof(int),1,f)!=1){ fclose(f); return false; }
        g_campaign.playerArmy.push_back({ti,cnt});
    }

    int nr=0;
    if(fread(&nr,sizeof(int),1,f)!=1||nr<0||nr>256){ fclose(f); return false; }
    g_campaign.readyUnits.clear();
    for(int i=0;i<nr;i++){
        int ti=0,cnt=0;
        if(fread(&ti,sizeof(int),1,f)!=1||fread(&cnt,sizeof(int),1,f)!=1){ fclose(f); return false; }
        g_campaign.readyUnits.push_back({ti,cnt});
    }

    // Fase J: ejércitos de campo + reserva
    g_campaign.armies.clear();
    g_campaign.reserve.clear();
    int nA=0;
    if(fread(&nA,sizeof(int),1,f)!=1||nA<0||nA>64){ fclose(f); return false; }
    for(int i=0;i<nA;i++){
        FieldArmy a;
        int id=0,prov=0,own=0,nu=0;
        unsigned char mv=0;
        if(fread(&id,sizeof(int),1,f)!=1||fread(&own,sizeof(int),1,f)!=1||
           fread(&prov,sizeof(int),1,f)!=1||fread(&mv,sizeof(unsigned char),1,f)!=1||
           fread(&nu,sizeof(int),1,f)!=1||nu<0||nu>256){ fclose(f); return false; }
        a.id=id; a.owner=(FactionId)own; a.province=prov; a.moved=(mv!=0);
        for(int j=0;j<nu;j++){
            int ti=0,cnt=0;
            if(fread(&ti,sizeof(int),1,f)!=1||fread(&cnt,sizeof(int),1,f)!=1){ fclose(f); return false; }
            a.units.push_back({ti,cnt});
        }
        g_campaign.armies.push_back(a);
        if(id>=g_campaign.nextArmyId) g_campaign.nextArmyId=id+1;
    }
    int nR=0;
    if(fread(&nR,sizeof(int),1,f)!=1||nR<0||nR>512){ fclose(f); return false; }
    for(int i=0;i<nR;i++){
        int ti=0,cnt=0;
        if(fread(&ti,sizeof(int),1,f)!=1||fread(&cnt,sizeof(int),1,f)!=1){ fclose(f); return false; }
        g_campaign.reserve.push_back({ti,cnt});
    }

    // Fase I: diplomacia — alianzas militares + pactos comerciales
    if(fread(g_campaign.allied,sizeof(bool),FACTION_COUNT,f)!=FACTION_COUNT){ fclose(f); return false; }
    if(fread(g_campaign.tradePact,sizeof(bool),FACTION_COUNT,f)!=FACTION_COUNT){ fclose(f); return false; }

    fclose(f);
    updateProvinceCenters();
    g_campaign.playerArmy = g_campaign.readyUnits; // 1.4: sync after load
    // Fase J: readyUnits es una vista del ejército seleccionado
    if(!g_campaign.armies.empty()) selectArmy(0);
    else { g_campaign.selectedArmy=-1; }
    g_campaign.playerArmy = g_campaign.readyUnits;
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
