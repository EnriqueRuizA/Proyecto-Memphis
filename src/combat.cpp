#include "combat.h"
#include "config.h"
#include "util.h"
#include "ui.h"
#include "sprite.h"
#include "terrain.h"
#include "camera.h"
#include "campaign.h"
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

BattleState  g_battle;      // (:645)
BattleResult g_lastResult;  // (:661)

std::deque<std::string> g_battleLog;
void battleLogAdd(const std::string& s){
    g_battleLog.push_front(s);
    if((int)g_battleLog.size()>50) g_battleLog.pop_back();
}

//  BATTLE INIT
// ═══════════════════════════════════════════════════════════════════════════
void initBattle(const std::vector<std::pair<int,int>>& playerGroups,
                        const std::vector<std::pair<int,int>>& enemyGroups,
                        TerrainType terrain,int provinceIdx,bool isDefense,
                        const char* scenarioName){
    g_battle={};
    g_battle.terrain=terrain;
    g_battle.battleProvince=provinceIdx;
    g_battle.isDefense=isDefense;
    g_battle.timeScale=1.f;
    g_battle.activeControlGroup=-1;
    strncpy(g_battle.scenarioName,scenarioName,sizeof(g_battle.scenarioName)-1); g_battle.scenarioName[sizeof(g_battle.scenarioName)-1]='\0';

    // Camera centered on player deployment zone (extraído a camera.cpp)
    initBattleCamera();

    // Generate terrain obstacles (extraído a terrain.cpp)
    generateTerrainObstacles(terrain, provinceIdx, g_battle.obstacles);

    // Player units — left side
    float py=BATTLE_H/2.f;
    float px=BATTLE_W*0.2f;
    for(int gi=0;gi<(int)playerGroups.size();gi++){
        auto [typeIdx,cnt]=playerGroups[gi];
        if(typeIdx>=unitTypeCount()||cnt<=0) continue;
        const UnitTypeDef& td=g_unitTypes[typeIdx];
        BattleUnit bu{};
        bu.typeIdx=typeIdx; bu.isPlayer=true;
        bu.anchorPos={px, py+(gi-playerGroups.size()/2.f)*90.f};
        bu.orderTarget=bu.anchorPos;
        bu.orderAttack=-1;
        bu.morale=td.morale;
        bu.groupState=UGS_IDLE;
        bu.veterancy=0;
        bu.aiReactTimer=0.f;
        bu.orderType=0; // 6.6: attack order type
        bu.formationFacing=0.f; // 6.6: facing angle (0 = right)
        bu.lastFormationFacing=0.f; // v7.2: initialize facing tracker
        int soldierCount=std::min(cnt,(int)td.soldierCount);
        auto slots=calcFormationSlots(bu.anchorPos,soldierCount,0.f);
        for(int s=0;s<soldierCount;s++){
            Soldier sol{};
            sol.pos=slots[s];
            sol.formationSlot=slots[s];
            sol.hp=td.hpPerSoldier;
            sol.alive=true;
            sol.angle=0; sol.angleTarget=0;
            sol.attackTargetSoldier=-1; sol.attackTargetUnit=-1;
            sol.state=SS_IDLE;
            sol.chargeReady=(td.spriteBase==SPR_CAVALRY);
            sol.targetRefreshTimer=0.f;
            sol.kills=0;
            bu.soldiers.push_back(sol);
        }
        g_battle.playerUnits.push_back(bu);
    }

    // Enemy units — right side
    float ey=BATTLE_H/2.f;
    float ex2=BATTLE_W*0.8f;
    for(int gi=0;gi<(int)enemyGroups.size();gi++){
        auto [typeIdx,cnt]=enemyGroups[gi];
        if(typeIdx>=unitTypeCount()||cnt<=0) continue;
        const UnitTypeDef& td=g_unitTypes[typeIdx];
        BattleUnit bu{};
        bu.typeIdx=typeIdx; bu.isPlayer=false;
        bu.anchorPos={ex2, ey+(gi-(int)enemyGroups.size()/2.f)*90.f};
        bu.orderTarget=bu.anchorPos;
        bu.orderAttack=-1;
        bu.morale=td.morale;
        bu.groupState=UGS_IDLE;
        bu.veterancy=0;
        bu.aiReactTimer=0.f;
        bu.orderType=0; // 6.6: attack order type
        bu.formationFacing=180.f; // 6.6: facing angle (180 = left for enemies)
        bu.lastFormationFacing=180.f; // v7.2: initialize facing tracker
        int soldierCount=std::min(cnt,(int)td.soldierCount);
        auto slots=calcFormationSlots(bu.anchorPos,soldierCount,180.f);
        for(int s=0;s<soldierCount;s++){
            Soldier sol{};
            sol.pos=slots[s];
            sol.formationSlot=slots[s];
            sol.hp=td.hpPerSoldier;
            sol.alive=true;
            sol.angle=180; sol.angleTarget=180;
            sol.attackTargetSoldier=-1; sol.attackTargetUnit=-1;
            sol.state=SS_IDLE;
            sol.chargeReady=(td.spriteBase==SPR_CAVALRY);
            sol.targetRefreshTimer=0.f;
            sol.kills=0;
            bu.soldiers.push_back(sol);
        }
        g_battle.enemyUnits.push_back(bu);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  COMBAT HELPERS
// ═══════════════════════════════════════════════════════════════════════════
float soldierMeleeRadius(const UnitTypeDef& td){
    return td.spriteBase==SPR_CAVALRY?16.f:10.f;
}

float calcMeleeDmg(const UnitTypeDef& attTd,const UnitTypeDef& defTd,bool chargeBonus){
    float hitChance=std::max(8.f,std::min(92.f,30.f+(float)attTd.meleeAttack-(float)defTd.meleeDefense));
    if(frandMT()*100.f>=hitChance) return 0.f;
    float armRed=frandMT()*0.5f*(float)defTd.armor+(float)defTd.armor*0.5f;
    armRed=std::min(armRed,(float)defTd.armor);
    float dmg=std::max(0.5f,attTd.meleeBaseDmg*(1.f-armRed/100.f)+attTd.meleeAPDmg);
    if(chargeBonus) dmg*=2.5f;
    return dmg;
}

float calcMissileDmg(const UnitTypeDef& attTd,const UnitTypeDef& defTd){
    float armRed=frandMT()*0.5f*(float)defTd.armor+(float)defTd.armor*0.5f;
    armRed=std::min(armRed,(float)defTd.armor);
    return std::max(0.f,attTd.missileBaseDmg*(1.f-armRed/100.f)+attTd.missileAPDmg);
}

// 6.6: Improved obstacle avoidance with steering behavior
Vector2 avoidObstacles(Vector2 pos, Vector2 targetDir, float speed, const std::vector<Rectangle>& obstacles){
    Vector2 ahead=v2add(pos,v2scale(targetDir,speed*0.5f));
    Vector2 ahead2=v2add(pos,v2scale(targetDir,speed*0.25f));

    // Check if we're hitting an obstacle
    Rectangle checkBox={ahead.x-8,ahead.y-8,16,16};
    Rectangle checkBox2={ahead2.x-8,ahead2.y-8,16,16};

    for(auto& obs:obstacles){
        if(CheckCollisionRecs(checkBox,obs)||CheckCollisionRecs(checkBox2,obs)){
            // Try to steer around obstacle
            Vector2 obsCenter={obs.x+obs.width/2,obs.y+obs.height/2};
            Vector2 toObs=vnorm(v2sub(obsCenter,pos));
            Vector2 perp={-toObs.y,toObs.x};
            // Steer perpendicular to obstacle
            return vnorm(v2add(targetDir,v2scale(perp,0.7f)));
        }
    }
    return targetDir;
}

// LOS check for ranged: returns true if line is clear enough to shoot
bool losClean(Vector2 from,Vector2 to,const std::vector<Soldier>& friendlies,int selfIdx){
    Vector2 dir=vnorm(v2sub(to,from));
    float dist=vdist(from,to);
    int blocked=0;
    for(int i=0;i<(int)friendlies.size();i++){
        if(i==selfIdx||!friendlies[i].alive) continue;
        Vector2 rel=v2sub(friendlies[i].pos,from);
        float proj=v2dot(rel,dir);
        if(proj<0||proj>dist) continue;
        float perp=fabsf(rel.x*(-dir.y)+rel.y*dir.x);
        if(perp<10.f){ blocked++; if(blocked>=2) return false; }
    }
    // Check terrain obstacles
    for(auto& obs:g_battle.obstacles){
        // Simple: check if line intersects rectangle
        // Rough check: if rect centroid is within 30px of line
        Vector2 rc={obs.x+obs.width/2,obs.y+obs.height/2};
        Vector2 rel=v2sub(rc,from);
        float proj=v2dot(rel,dir);
        if(proj>0&&proj<dist){
            float perp=fabsf(rel.x*(-dir.y)+rel.y*dir.x);
            if(perp<(obs.width+obs.height)*0.25f) return false;
        }
    }
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
//  BATTLE UPDATE
// ═══════════════════════════════════════════════════════════════════════════
void separateSoldiers(std::vector<BattleUnit>& units){
    // Separation within same side
    float minD=18.f;
    for(auto& bu:units){
        for(int i=0;i<(int)bu.soldiers.size();i++){
            if(!bu.soldiers[i].alive) continue;
            // Against same unit
            for(int j=i+1;j<(int)bu.soldiers.size();j++){
                if(!bu.soldiers[j].alive) continue;
                float d=vdist(bu.soldiers[i].pos,bu.soldiers[j].pos);
                if(d<minD&&d>0.001f){
                    float push=(minD-d)*0.35f;
                    Vector2 dir2=vnorm(v2sub(bu.soldiers[j].pos,bu.soldiers[i].pos));
                    bu.soldiers[i].pos=v2sub(bu.soldiers[i].pos,v2scale(dir2,push));
                    bu.soldiers[j].pos=v2add(bu.soldiers[j].pos,v2scale(dir2,push));
                }
            }
            // Clamp
            bu.soldiers[i].pos.x=std::max(8.f,std::min((float)BATTLE_W-8,bu.soldiers[i].pos.x));
            bu.soldiers[i].pos.y=std::max(8.f,std::min((float)BATTLE_H-8,bu.soldiers[i].pos.y));
        }
    }
}

// Find nearest alive soldier in an enemy BattleUnit list, returns {unitIdx, solIdx} or {-1,-1}
std::pair<int,int> findNearestEnemySoldier(Vector2 from,
        const std::vector<BattleUnit>& enemies){
    float best=1e9f; int bu2=-1,si=-1;
    for(int u=0;u<(int)enemies.size();u++){
        for(int s=0;s<(int)enemies[u].soldiers.size();s++){
            if(!enemies[u].soldiers[s].alive) continue;
            float d=vdist(from,enemies[u].soldiers[s].pos);
            if(d<best){best=d;bu2=u;si=s;}
        }
    }
    return {bu2,si};
}

void updateBattleUnits(std::vector<BattleUnit>& myUnits,
                               std::vector<BattleUnit>& foeUnits,
                               float dt){
    for(int ui=0;ui<(int)myUnits.size();ui++){
        BattleUnit& bu=myUnits[ui];
        if(bu.typeIdx>=unitTypeCount()) continue;
        const UnitTypeDef& td=g_unitTypes[bu.typeIdx];

        int aliveCnt=0;
        for(auto& sol:bu.soldiers) if(sol.alive) aliveCnt++;
        if(aliveCnt==0) continue;

        // Morale update
        float totalSol=(float)td.soldierCount;
        if((float)aliveCnt/totalSol < 0.4f){
            bu.morale-=1.f*dt;
        }
        bu.moraleTimer+=dt;
        if(bu.moraleTimer>4.f&&bu.morale<td.morale){
            bu.morale=std::min(td.morale,bu.morale+0.5f*60.f*dt);
        }
        bu.morale=std::max(0.f,std::min(100.f,bu.morale));

        // Routing logic
        if(bu.groupState==UGS_ROUTING){
            bu.routeTimer-=dt;
            if(bu.routeTimer<=0){
                if(bu.morale<5.f){
                    // Permanently rout — kill all soldiers
                    for(auto& sol:bu.soldiers) sol.alive=false;
                } else {
                    bu.groupState=UGS_IDLE;
                }
            }
            // Soldiers flee
            // Compute enemy centroid
            Vector2 foeCenter={0,0}; int foeCount=0;
            for(auto& fu:foeUnits) for(auto& fs:fu.soldiers) if(fs.alive){foeCenter=v2add(foeCenter,fs.pos);foeCount++;}
            if(foeCount>0){foeCenter=v2scale(foeCenter,1.f/foeCount);}
            for(auto& sol:bu.soldiers){
                if(!sol.alive) continue;
                sol.state=SS_FLEEING;
                Vector2 away=vnorm(v2sub(sol.pos,foeCenter));
                sol.pos=v2add(sol.pos,v2scale(away,td.speed*1.5f*dt));
                sol.angleTarget=dirToAngle(away);
                sol.angle=lerpAngle(sol.angle,sol.angleTarget,8.f*dt);
            }
            continue;
        }
        if(bu.morale<20.f&&bu.groupState!=UGS_ROUTING){
            bu.groupState=UGS_ROUTING;
            bu.routeTimer=6.f;
        }

        // ═══════════════════════════════════════════════════════════════════════════
        //  MOVEMENT SYSTEM v8.1: ANCHOR FOLLOWS CENTROID (STABLE)
        // ═══════════════════════════════════════════════════════════════════════════

        // Determine group order target (explicit attack or move)
        bool hasAttackOrder=(bu.orderAttack>=0&&bu.orderAttack<(int)foeUnits.size());
        bool hasExplicitMoveOrder=(bu.hasExplicitOrder&&bu.orderType==1);

        // Calculate desired movement (toward order target or auto-advance to enemies)
        Vector2 desiredAnchor=bu.anchorPos;

        if(hasExplicitMoveOrder){
            Vector2 moveDir=vnorm(v2sub(bu.orderTarget,bu.anchorPos));
            float distToTarget=vdist(bu.anchorPos,bu.orderTarget);

            if(distToTarget>15.f){
                desiredAnchor=v2add(bu.anchorPos,v2scale(moveDir,td.speed*dt));
                bu.formationFacing=dirToAngle(moveDir);
            } else {
                // Arrived at target
                desiredAnchor=bu.orderTarget;
                bu.hasExplicitOrder=false;
            }
        } 
        else if(hasAttackOrder||(!hasExplicitMoveOrder&&!bu.hasExplicitOrder&&foeUnits.size()>0)){
            // Auto-advance toward nearest enemy when no explicit orders
            // FIX: find nearest enemy group, not always index 0
            int targetIdx=hasAttackOrder?bu.orderAttack:0;
            if(!hasAttackOrder){
                float bestDist=1e9f;
                for(int ei=0;ei<(int)foeUnits.size();ei++){
                    int eAlive=0; for(auto& es:foeUnits[ei].soldiers) if(es.alive) eAlive++;
                    if(eAlive==0) continue;
                    float d=vdist(bu.anchorPos,foeUnits[ei].anchorPos);
                    if(d<bestDist){bestDist=d;targetIdx=ei;}
                }
            }
            if(validIdx(targetIdx,foeUnits)){
                Vector2 toEnemy=vnorm(v2sub(foeUnits[targetIdx].anchorPos,bu.anchorPos));
                float distToEnemy=vdist(bu.anchorPos,foeUnits[targetIdx].anchorPos);

                // Move toward enemy at full speed until close engagement (v8.2: AGGRESSIVE)
                // This prevents units from orbiting instead of engaging
                float engagementDistance=70.f; // Reduced from 100px for faster engagement
                if(distToEnemy>engagementDistance){
                    desiredAnchor=v2add(bu.anchorPos,v2scale(toEnemy,td.speed*dt));
                }

                // Rotate formation to face enemy
                bu.formationFacing=dirToAngle(toEnemy);
            }
        }

        // FIX: ALWAYS recalculate slots using desiredAnchor (the intended new position).
        // The old guard (>5 degree facing change) caused slots to go stale during straight-line
        // movement: anchor advanced but soldiers chased old world positions → wrong direction / trembling.
        {
            bu.lastFormationFacing=bu.formationFacing;
            auto slots=calcFormationSlots(desiredAnchor,(int)bu.soldiers.size(),bu.formationFacing);
            for(int s=0;s<(int)bu.soldiers.size()&&s<(int)slots.size();s++){
                bu.soldiers[s].formationSlot=slots[s];
            }
        }

        // ═══════════════════════════════════════════════════════════════════════════
        //  INDIVIDUAL SOLDIER UPDATES (v8.0: SMOOTH GRAVITATIONAL PULL)
        // ═══════════════════════════════════════════════════════════════════════════

        // Calculate frontline threshold ONCE per unit (optimization + stability)
        float frontlineDistance=1e9f;
        for(auto& eneunit:foeUnits){
            for(auto& esol:eneunit.soldiers){
                if(!esol.alive) continue;
                for(auto& sol:bu.soldiers){
                    if(!sol.alive) continue;
                    float d=vdist(sol.pos,esol.pos);
                    frontlineDistance=std::min(frontlineDistance,d);
                }
            }
        }
        if(frontlineDistance==1e9f) frontlineDistance=200.f;
        float frontlineThreshold=frontlineDistance+50.f;

        // Each soldier updates individually
        for(int si=0;si<(int)bu.soldiers.size();si++){
            Soldier& sol=bu.soldiers[si];
            if(!sol.alive) continue;

            sol.meleeTimer-=dt;
            sol.rangeTimer-=dt;
            sol.inMelee=false;

            // Check if this soldier is in front line
            float distToClosestEnemy=1e9f;
            for(auto& eneunit:foeUnits){
                for(auto& esol:eneunit.soldiers){
                    if(!esol.alive) continue;
                    float d=vdist(sol.pos,esol.pos);
                    distToClosestEnemy=std::min(distToClosestEnemy,d);
                }
            }
            bool inFrontLine=(distToClosestEnemy<=frontlineThreshold);

            // Resolve attack target
            int targetUnit=sol.attackTargetUnit;
            int targetSol=sol.attackTargetSoldier;
            bool targetValid=(targetUnit>=0&&targetUnit<(int)foeUnits.size()
                              &&targetSol>=0&&targetSol<(int)foeUnits[targetUnit].soldiers.size()
                              &&foeUnits[targetUnit].soldiers[targetSol].alive);

            if(!targetValid){
                sol.targetRefreshTimer-=dt;
                bool needRefresh=(sol.targetRefreshTimer<=0.f);
                if(!needRefresh&&sol.attackTargetUnit>=0){
                    if(validIdx(sol.attackTargetUnit,foeUnits)&&
                       validIdx(sol.attackTargetSoldier,foeUnits[sol.attackTargetUnit].soldiers)&&
                       foeUnits[sol.attackTargetUnit].soldiers[sol.attackTargetSoldier].alive){
                        targetUnit=sol.attackTargetUnit;
                        targetSol=sol.attackTargetSoldier;
                        targetValid=true;
                    } else needRefresh=true;
                }
                if(needRefresh){
                    sol.targetRefreshTimer=0.3f;
                    if(hasAttackOrder){
                        float best=1e9f; int bs=-1;
                        for(int s2=0;s2<(int)foeUnits[bu.orderAttack].soldiers.size();s2++){
                            if(!foeUnits[bu.orderAttack].soldiers[s2].alive) continue;
                            float d=vdist(sol.pos,foeUnits[bu.orderAttack].soldiers[s2].pos);
                            if(d<best){best=d;bs=s2;}
                        }
                        if(bs>=0){targetUnit=bu.orderAttack;targetSol=bs;targetValid=true;}
                    }
                    if(!targetValid&&!bu.hasExplicitOrder){
                        auto [nu,ns]=findNearestEnemySoldier(sol.pos,foeUnits);
                        if(nu>=0){targetUnit=nu;targetSol=ns;targetValid=true;}
                    }
                    sol.attackTargetUnit=targetUnit;
                    sol.attackTargetSoldier=targetSol;
                }
            }

            // ═══════════════════════════════════════════════════════════════════════════
            //  SOLDIER MOVEMENT PRIORITY SYSTEM v8.0
            // ═══════════════════════════════════════════════════════════════════════════

            float slotDist=vdist(sol.pos,sol.formationSlot);
            const float SLOT_TOLERANCE=8.f; // When soldier is "in slot" (strict precision)

            // If no target, move to formation slot instead of idle
            if(!targetValid){
                sol.state=SS_MOVING_SLOT;

                if(slotDist>SLOT_TOLERANCE){
                    Vector2 toSlot=vnorm(v2sub(sol.formationSlot,sol.pos));
                    toSlot=avoidObstacles(sol.pos,toSlot,td.speed,g_battle.obstacles);
                    // v8.2: Fast movement - linear instead of deceleration curve
                    // Soldiers move at FULL speed toward their slot
                    sol.pos=v2add(sol.pos,v2scale(toSlot,td.speed*dt));
                    sol.angleTarget=dirToAngle(toSlot);
                    sol.chargeMoveTime+=dt;
                    if(sol.chargeMoveTime>0.8f) sol.chargeReady=(td.spriteBase==SPR_CAVALRY);
                } else {
                    sol.state=SS_IDLE;
                    sol.chargeMoveTime=0.f;
                }
                sol.angle=lerpAngle(sol.angle,sol.angleTarget,10.f*dt);
                continue;  // Skip combat logic if no target
            }

            Soldier& tgt=foeUnits[targetUnit].soldiers[targetSol];
            const UnitTypeDef& fTd=g_unitTypes[foeUnits[targetUnit].typeIdx];
            float myRad=soldierMeleeRadius(td);
            float fRad=soldierMeleeRadius(fTd);
            float meleeD=myRad+fRad+6.f;
            float dist=vdist(sol.pos,tgt.pos);
            Vector2 dir2=vnorm(v2sub(tgt.pos,sol.pos));
            sol.angleTarget=dirToAngle(dir2);

            // Priority 1: MELEE COMBAT (front line only) - BREAK FORMATION
            if(inFrontLine&&dist<=meleeD){
                sol.inMelee=true;
                sol.state=SS_ATTACKING_MELEE;

                // FIX: radius fixed per soldier index (no random per frame), time from GetTime()
                float orbitRadius=20.f+(float)(si%4)*4.f;
                float orbitSpeed=2.5f;
                float timeFactor=(float)GetTime()+(float)si*0.7f;
                float orbitAngle=timeFactor*orbitSpeed;
                Vector2 orbitPos={
                    tgt.pos.x + cosf(orbitAngle)*orbitRadius,
                    tgt.pos.y + sinf(orbitAngle)*orbitRadius
                };

                // Move toward orbital position (not rigid slot)
                Vector2 toOrbit=vnorm(v2sub(orbitPos,sol.pos));
                sol.pos=v2add(sol.pos,v2scale(toOrbit,td.speed*0.4f*dt));  // Slower movement in melee

                if(sol.meleeTimer<=0){
                    bool chargeBonus=sol.chargeReady&&td.spriteBase==SPR_CAVALRY;
                    float dmg=calcMeleeDmg(td,fTd,chargeBonus);
                    if(chargeBonus&&dmg>0){
                        sol.chargeReady=false;
                        tgt.pos=v2add(tgt.pos,v2scale(dir2,12.f));
                        foeUnits[targetUnit].morale-=10.f;
                    }
                    tgt.hp-=dmg;
                    if(tgt.hp<=0){
                        tgt.alive=false;
                        sol.kills++;
                        g_battle.dead.push_back({tgt.pos,1.f,8.f,
                            fTd.spriteBase==SPR_CAVALRY,tgt.angle,
                            foeUnits[targetUnit].typeIdx,
                            foeUnits[targetUnit].isPlayer?0:1});
                        if(fTd.soldierCount>0){
                            char logbuf[128];
                            snprintf(logbuf,127,"[T%d] %s soldier slain by %s",
                                g_campaign.turn,fTd.name,td.name);
                            battleLogAdd(logbuf);
                        }
                        for(int fui=0;fui<(int)foeUnits.size();fui++){
                            for(auto& fs:foeUnits[fui].soldiers){
                                if(!fs.alive) continue;
                                if(vdist(tgt.pos,fs.pos)<80.f) foeUnits[fui].morale-=3.f;
                            }
                        }
                        sol.attackTargetSoldier=-1; sol.attackTargetUnit=-1;
                        sol.targetRefreshTimer=0.f;
                    }
                    sol.meleeTimer=td.meleeInterval;
                }
                sol.chargeMoveTime=0.f;
            }
            // Priority 2: RANGED COMBAT (back line support, not in melee)
            else if(td.range>0&&dist<=(float)td.range&&!sol.inMelee&&sol.rangeTimer<=0){
                if(losClean(sol.pos,tgt.pos,bu.soldiers,si)){
                    sol.state=SS_ATTACKING_RANGED;
                    float dmg=calcMissileDmg(td,fTd);
                    bool isCross=(td.missileReload>2.5f);
                    float projSpeed=isCross?380.f:320.f;
                    g_battle.projs.push_back({sol.pos,v2scale(dir2,projSpeed),dmg,true,bu.isPlayer,isCross});
                    sol.rangeTimer=td.missileReload;
                    sol.chargeMoveTime=0.f;
                }
            }
            // Priority 3: MOVE TOWARD SLOT IF ENEMY OUT OF RANGE
            else {
                sol.state=SS_MOVING_SLOT;
                float slotDist=vdist(sol.pos,sol.formationSlot);

                if(slotDist>8.f){
                    Vector2 toSlot=vnorm(v2sub(sol.formationSlot,sol.pos));
                    toSlot=avoidObstacles(sol.pos,toSlot,td.speed,g_battle.obstacles);
                    // v8.2: Fast movement - no deceleration curve
                    sol.pos=v2add(sol.pos,v2scale(toSlot,td.speed*dt));
                    sol.angleTarget=dirToAngle(toSlot);
                    sol.chargeMoveTime+=dt;
                    if(sol.chargeMoveTime>0.8f) sol.chargeReady=(td.spriteBase==SPR_CAVALRY);
                } else {
                    sol.state=SS_IDLE;
                    sol.chargeMoveTime=0.f;
                }
            }

            sol.angle=lerpAngle(sol.angle,sol.angleTarget,10.f*dt);
        }

        // Update group state based on soldier activity
        bool anyMelee=false,anyMoving=false;
        for(auto& sol:bu.soldiers){
            if(!sol.alive) continue;
            if(sol.inMelee) anyMelee=true;
            if(sol.state==SS_MOVING_SLOT) anyMoving=true;
        }
        if(anyMelee) bu.groupState=UGS_ENGAGED;
        else if(anyMoving) bu.groupState=UGS_ADVANCING;
        else bu.groupState=UGS_IDLE;

        // ═══════════════════════════════════════════════════════════════════════════
        //  ANCHOR UPDATE v8.1: FOLLOW CENTROID OF SOLDIERS
        // ═══════════════════════════════════════════════════════════════════════════
        // Calculate actual centroid of all alive soldiers
        Vector2 actualCentroid={0,0};
        int aliveCnt2=0;
        for(auto& sol:bu.soldiers){
            if(sol.alive){
                actualCentroid=v2add(actualCentroid,sol.pos);
                aliveCnt2++;
            }
        }
        if(aliveCnt2>0) actualCentroid=v2scale(actualCentroid,1.f/(float)aliveCnt2);
        else actualCentroid=bu.anchorPos; // fallback if all dead

        // Move anchor smoothly toward desired position (toward movement target or auto-advance)
        // MINIMAL centroid correction - anchor leads, soldiers follow
        Vector2 targetAnchor=desiredAnchor;

        // v8.3: ULTRA-AGGRESSIVE ADVANCE - almost pure desired movement
        // Only use 10% centroid correction to prevent oscillation
        // This allows anchor to lead units toward enemy without drag
        float centroidWeight=0.1f;  // Minimal correction for all movement types
        targetAnchor.x=targetAnchor.x*(1.f-centroidWeight)+actualCentroid.x*centroidWeight;
        targetAnchor.y=targetAnchor.y*(1.f-centroidWeight)+actualCentroid.y*centroidWeight;

        // Snap to target anchor (v8.3: INSTANT RESPONSE)
        // At 60 FPS: 50.0 * 0.016 = 80% per frame (1-2 frames convergence)
        bu.anchorPos.x+=(targetAnchor.x-bu.anchorPos.x)*50.f*dt;
        bu.anchorPos.y+=(targetAnchor.y-bu.anchorPos.y)*50.f*dt;
    }
}

// Enemy AI: assign orders periodically (3.2: aiReactTimer, differentiated behaviors)
void updateEnemyAI(float dt){
    int nPlayerAlive=0;
    for(auto& pu:g_battle.playerUnits){
        int ac=0; for(auto& s:pu.soldiers) if(s.alive) ac++;
        if(ac>0) nPlayerAlive++;
    }
    if(nPlayerAlive==0) return;

    for(int ui=0;ui<(int)g_battle.enemyUnits.size();ui++){
        BattleUnit& bu=g_battle.enemyUnits[ui];
        if(bu.groupState==UGS_ROUTING) continue;
        int aliveCnt=0;
        for(auto& sol:bu.soldiers) if(sol.alive) aliveCnt++;
        if(aliveCnt==0) continue;

        // Tick AI react timer
        bu.aiReactTimer-=dt;
        if(bu.aiReactTimer>0) continue; // skip recalculation this frame
        bu.aiReactTimer=1.5f; // recalculate every 1.5s

        if(bu.typeIdx>=unitTypeCount()) continue;
        const UnitTypeDef& td=g_unitTypes[bu.typeIdx];

        // Find nearest player group
        float best=1e9f; int best_u=-1;
        for(int pu=0;pu<(int)g_battle.playerUnits.size();pu++){
            int pAlive=0;
            for(auto& ps:g_battle.playerUnits[pu].soldiers) if(ps.alive) pAlive++;
            if(pAlive==0) continue;
            float d=vdist(bu.anchorPos,g_battle.playerUnits[pu].anchorPos);
            if(d<best){best=d;best_u=pu;}
        }
        if(best_u<0) continue;

        Vector2 targetPos=g_battle.playerUnits[best_u].anchorPos;
        float distToTarget=vdist(bu.anchorPos,targetPos);

        if(td.range>0){
            // 3.2: Ranged — maintain 200px distance, retreat if <150px
            if(distToTarget<150.f){
                // Retreat
                Vector2 away=vnorm(v2sub(bu.anchorPos,targetPos));
                bu.orderTarget=v2add(bu.anchorPos,v2scale(away,200.f));
                bu.orderAttack=-1;
                bu.hasExplicitOrder=true;
                bu.orderType=1; // move
                // FIX: removed bu.anchorPos=bu.orderTarget (was teleporting)
            } else if(distToTarget>220.f){
                bu.orderAttack=best_u;
                bu.orderType=0; // attack
                bu.hasExplicitOrder=false;
            } else {
                // Hold — just attack in range
                bu.orderAttack=best_u;
                bu.orderType=2; // hold
                bu.hasExplicitOrder=false;
            }
        } else if(td.spriteBase==SPR_CAVALRY){
            // 3.2: Cavalry — flanking angle 45-90 degrees lateral
            Vector2 toTarget=vnorm(v2sub(targetPos,bu.anchorPos));
            float flankAngle=DEG2RAD*(45.f+frandMT()*45.f);
            float side=(ui%2==0)?1.f:-1.f;
            Vector2 flankDir={
                toTarget.x*cosf(flankAngle*side)-toTarget.y*sinf(flankAngle*side),
                toTarget.x*sinf(flankAngle*side)+toTarget.y*cosf(flankAngle*side)
            };
            bu.orderTarget=v2add(targetPos,v2scale(flankDir,80.f));
            bu.orderAttack=best_u;
            bu.orderType=3; // flank
            bu.hasExplicitOrder=true;
            bu.anchorPos=v2add(bu.anchorPos,v2scale(vnorm(v2sub(bu.orderTarget,bu.anchorPos)),5.f));
        } else {
            // 3.2: Infantry — distribute across different player groups if multiple exist
            if(nPlayerAlive>1){
                // Spread: pick target based on our index
                int targetIdx=ui % nPlayerAlive;
                int counted=0;
                for(int pu=0;pu<(int)g_battle.playerUnits.size();pu++){
                    int ac=0; for(auto& ps:g_battle.playerUnits[pu].soldiers) if(ps.alive) ac++;
                    if(ac>0){
                        if(counted==targetIdx){best_u=pu;break;}
                        counted++;
                    }
                }
            }
            bu.orderAttack=best_u;
            bu.orderType=0; // attack
            bu.hasExplicitOrder=false;
        }

        // Update formation slots toward target (6.6: use proper facing angle)
        if(bu.orderAttack>=0&&validIdx(bu.orderAttack,g_battle.playerUnits)){
            auto slots=calcFormationSlots(bu.anchorPos,(int)bu.soldiers.size(),bu.formationFacing);
            for(int s=0;s<(int)bu.soldiers.size()&&s<(int)slots.size();s++)
                bu.soldiers[s].formationSlot=slots[s];
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  BATTLE DRAW
// ═══════════════════════════════════════════════════════════════════════════
void drawBattlefield(){
    // Terrain background
    ClearBackground(C_TERRAIN);
    // Variation noise (static, initialized once)
    static bool terrainInit=false;
    static Image terrainNoise;
    static Texture2D terrainTex={0};
    if(!terrainInit){
        terrainNoise=GenImageColor(BATTLE_W,BATTLE_H,C_TERRAIN);
        srand(42);
        for(int y2=0;y2<BATTLE_H;y2+=8){
            for(int x2=0;x2<BATTLE_W;x2+=8){
                int v=rand()%12-6;
                Color c2={clampU8((int)C_TERRAIN.r+v),clampU8((int)C_TERRAIN.g+v),clampU8((int)C_TERRAIN.b+v),255};
                ImageDrawRectangle(&terrainNoise,x2,y2,8,8,c2);
            }
        }
        terrainTex=LoadTextureFromImage(terrainNoise);
        UnloadImage(terrainNoise);
        terrainInit=true; // 1.1: was missing, caused re-generation every frame
    }
    DrawTexture(terrainTex,0,0,WHITE);

    // Grid
    for(int x2=0;x2<BATTLE_W;x2+=128)
        DrawLine(x2,0,x2,BATTLE_H,{35,46,26,80});
    for(int y2=0;y2<BATTLE_H;y2+=128)
        DrawLine(0,y2,BATTLE_W,y2,{35,46,26,80});

    // Center line
    DrawLine(BATTLE_W/2,0,BATTLE_W/2,BATTLE_H,{50,70,35,100});

    // Obstacles
    for(auto& obs:g_battle.obstacles){
        Color oc;
        if(g_battle.terrain==TERRAIN_FOREST) oc={25,55,20,200};
        else oc={90,80,70,200};
        DrawRectangleRec(obs,oc);
        DrawRectangleLinesEx(obs,2,{20,45,15,255});
    }

    // Deployment zones (faint)
    DrawRectangle(0,0,(int)(BATTLE_W*0.35f),BATTLE_H,{80,120,220,12});
    DrawRectangle((int)(BATTLE_W*0.65f),0,(int)(BATTLE_W*0.35f),BATTLE_H,{220,60,50,12});
}

void drawAllUnits(){
    // Reloj de animación (se congela en pausa; con speed x2 corre al doble)
    static float s_animClock=0.f;
    s_animClock+=GetFrameTime()*(g_battle.paused?0.f:g_battle.timeScale);

    // Dead markers
    for(auto& d:g_battle.dead){
        // Hoja iso de muerte (Fase 2); si no, el blob generico
        if(drawUnitDeath(d.typeIdx,d.team,d.pos,d.angle,0.65f,8.f-d.timer,d.alpha))
            continue;
        int a=(int)(d.alpha*160);
        Color dc={60,20,20,(unsigned char)a};
        if(d.isCavalry){
            DrawEllipse((int)d.pos.x,(int)d.pos.y,12,8,dc);
        } else {
            DrawCircleV(d.pos,7,dc);
        }
        DrawLine((int)(d.pos.x-8),(int)(d.pos.y-8),(int)(d.pos.x+8),(int)(d.pos.y+8),{80,20,20,(unsigned char)a});
        DrawLine((int)(d.pos.x+8),(int)(d.pos.y-8),(int)(d.pos.x-8),(int)(d.pos.y+8),{80,20,20,(unsigned char)a});
    }

    // Projectiles
    for(auto& p:g_battle.projs){
        if(!p.alive) continue;
        Color pc=p.isCrossbow?Color{150,140,100,255}:Color{180,140,60,255};
        if(p.fromPlayer) pc={pc.r,(unsigned char)(pc.g+20),pc.b,255};
        DrawCircleV(p.pos,3,pc);
    }

    // Draw order lines for selected player units
    for(auto& bu:g_battle.playerUnits){
        if(!bu.selected) continue;
        // Show current orders visually
        if(bu.orderAttack>=0&&bu.orderAttack<(int)g_battle.enemyUnits.size()){
            // Attack order — red line to target
            DrawLineEx(bu.anchorPos,g_battle.enemyUnits[bu.orderAttack].anchorPos,2.5f,{255,80,80,160});
            // Arrow marker on enemy
            Vector2 dir=vnorm(v2sub(g_battle.enemyUnits[bu.orderAttack].anchorPos,bu.anchorPos));
            Vector2 markerPos=v2sub(g_battle.enemyUnits[bu.orderAttack].anchorPos,v2scale(dir,30.f));
            DrawCircleV(markerPos,8,{255,50,50,200});
            DrawText("⚔",(int)(markerPos.x-8),(int)(markerPos.y-8),16,{255,100,100,255});
        } else if(bu.hasExplicitOrder&&bu.orderType==1){
            // Move order — blue line to destination
            DrawLineEx(bu.anchorPos,bu.orderTarget,2.5f,{100,180,255,160});
            // Target marker
            DrawCircleLines((int)bu.orderTarget.x,(int)bu.orderTarget.y,12,{100,180,255,220});
            DrawText("→",(int)(bu.orderTarget.x-8),(int)(bu.orderTarget.y-6),16,{100,200,255,255});
        }
    }

    // Draw enemy units
    for(auto& bu:g_battle.enemyUnits){
        if(bu.typeIdx>=unitTypeCount()) continue;
        const UnitTypeDef& td=g_unitTypes[bu.typeIdx];
        int alive=0; for(auto& s:bu.soldiers) if(s.alive) alive++;
        if(alive==0) continue;
        // Group bounding circle
        DrawCircleLines((int)bu.anchorPos.x,(int)bu.anchorPos.y,
                        (int)(soldierMeleeRadius(td)*(float)alive*0.4f+20.f),
                        {180,40,40,40});
        for(auto& sol:bu.soldiers){
            if(!sol.alive) continue;
            int anim=SS_MOVING_SLOT==sol.state||SS_MOVING_TARGET==sol.state||
                     SS_FLEEING==sol.state?UA_WALK:
                     (sol.state==SS_ATTACKING_MELEE||sol.state==SS_ATTACKING_RANGED)?
                     UA_ATTACK:UA_IDLE;
            float phase=fmodf(sol.pos.x*0.37f+sol.pos.y*0.73f,10.f); // desincroniza
            drawUnit(bu.typeIdx,1,sol.pos,sol.angle,0.65f,true,true,
                     anim,s_animClock+phase);
            // 3.5: HP bar only when damaged
            if(sol.hp<td.hpPerSoldier){
                float bw=8.f; float ratio=sol.hp/td.hpPerSoldier;
                DrawRectangle((int)(sol.pos.x-bw/2),(int)(sol.pos.y-unitHeadOffset(bu.typeIdx,0.65f)-3),(int)bw,2,DARKGRAY);
                Color hc=ratio>0.5f?Color{0,228,48,255}:ratio>0.25f?Color{253,249,0,255}:Color{230,41,55,255};
                DrawRectangle((int)(sol.pos.x-bw/2),(int)(sol.pos.y-unitHeadOffset(bu.typeIdx,0.65f)-3),(int)(bw*ratio),2,hc);
            }
        }
        // Morale bar
        float mr=bu.morale/100.f;
        DrawRectangle((int)(bu.anchorPos.x-30),(int)(bu.anchorPos.y-28),60,5,DARKGRAY);
        DrawRectangle((int)(bu.anchorPos.x-30),(int)(bu.anchorPos.y-28),(int)(60*mr),5,
                      mr>0.5f?Color{255,161,0,255}:mr>0.2f?Color{253,249,0,255}:Color{230,41,55,255});
    }

    // Draw player units
    for(auto& bu:g_battle.playerUnits){
        if(bu.typeIdx>=unitTypeCount()) continue;
        const UnitTypeDef& td=g_unitTypes[bu.typeIdx];
        int alive=0; for(auto& s:bu.soldiers) if(s.alive) alive++;
        if(alive==0) continue;
        if(bu.selected){
            // 3.10: Pulsing selection circle
            float pulse=soldierMeleeRadius(td)*(float)alive*0.4f+24.f+sinf(g_menuTime*4.f)*4.f;
            DrawCircleLines((int)bu.anchorPos.x,(int)bu.anchorPos.y,
                            (int)pulse,
                            {C_ALLY.r,C_ALLY.g,C_ALLY.b,150});
            // 3.10: Soldier count above anchor
            char cntbuf[16]; snprintf(cntbuf,15,"%d",alive);
            int ctw=MeasureText(cntbuf,11);
            DrawText(cntbuf,(int)(bu.anchorPos.x-ctw/2),(int)(bu.anchorPos.y-pulse-14),11,C_ALLY);
        }
        for(auto& sol:bu.soldiers){
            if(!sol.alive) continue;
            int anim=SS_MOVING_SLOT==sol.state||SS_MOVING_TARGET==sol.state||
                     SS_FLEEING==sol.state?UA_WALK:
                     (sol.state==SS_ATTACKING_MELEE||sol.state==SS_ATTACKING_RANGED)?
                     UA_ATTACK:UA_IDLE;
            float phase=fmodf(sol.pos.x*0.37f+sol.pos.y*0.73f,10.f); // desincroniza
            drawUnit(bu.typeIdx,0,sol.pos,sol.angle,0.65f,bu.selected,true,
                     anim,s_animClock+phase);
            // 3.5: HP bar only when damaged
            if(sol.hp<td.hpPerSoldier){
                float bw=8.f; float ratio=sol.hp/td.hpPerSoldier;
                DrawRectangle((int)(sol.pos.x-bw/2),(int)(sol.pos.y-unitHeadOffset(bu.typeIdx,0.65f)-3),(int)bw,2,DARKGRAY);
                Color hc=ratio>0.5f?Color{0,228,48,255}:ratio>0.25f?Color{253,249,0,255}:Color{230,41,55,255};
                DrawRectangle((int)(sol.pos.x-bw/2),(int)(sol.pos.y-unitHeadOffset(bu.typeIdx,0.65f)-3),(int)(bw*ratio),2,hc);
            }
        }
        // 6.2: Veterancy stars above anchor
        if(bu.veterancy>0){
            for(int v=0;v<bu.veterancy;v++){
                DrawText("★",(int)(bu.anchorPos.x-10+v*10),(int)(bu.anchorPos.y-36),12,C_GOLD);
            }
        }
        // Control group badge: show which group(s) this unit belongs to
        if(bu.isPlayer){
            for(int g=0;g<9;g++){
                const ControlGroup& cg=g_battle.controlGroups[g];
                if(!cg.active) continue;
                for(int idx:cg.unitIndices){
                    if(validIdx(idx,g_battle.playerUnits)&&&g_battle.playerUnits[idx]==&bu){
                        // Draw group number badge
                        bool isActive=(g_battle.activeControlGroup==g);
                        Color bgc=isActive?Color{200,165,80,230}:Color{30,30,30,180};
                        Color fgc=isActive?Color{20,10,0,255}:Color{180,160,100,255};
                        int bx=(int)(bu.anchorPos.x+22);
                        int by=(int)(bu.anchorPos.y-42);
                        DrawRectangle(bx,by,14,14,bgc);
                        DrawRectangleLinesEx({(float)bx,(float)by,14,14},1,{150,120,50,200});
                        DrawText(TextFormat("%d",g+1),bx+4,by+2,10,fgc);
                        break;
                    }
                }
            }
        }
        // Morale bar
        float mr=bu.morale/100.f;
        DrawRectangle((int)(bu.anchorPos.x-30),(int)(bu.anchorPos.y-28),60,5,DARKGRAY);
        DrawRectangle((int)(bu.anchorPos.x-30),(int)(bu.anchorPos.y-28),(int)(60*mr),5,
                      mr>0.5f?Color{0,158,47,255}:mr>0.2f?Color{253,249,0,255}:Color{230,41,55,255});
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  HUD DRAW (battle)
// ═══════════════════════════════════════════════════════════════════════════
void drawBattleHUD(Vector2 mouse){
    int hudY=SCREEN_H-HUD_H;

    // Top bar
    DrawRectangle(0,0,SCREEN_W,TOPBAR_H,{0,0,0,220});
    DrawText(g_battle.scenarioName,10,8,14,C_GOLD);
    DrawText(TextFormat("Turn: %d  |  [SPACE] Speed  |  [ESC] Pause  |  [Ctrl+1-9] Save Group  |  [1-9] Select Group",
             g_campaign.turn),
             SCREEN_W/2-360,8,10,C_SECONDARY);
    DrawText("Right-click unit/location to command — Group move preserves formation",
             SCREEN_W/2-280, 20, 9, Color{180, 160, 120, 200});
    // Draw active control group badges on right of top bar
    {
        int bx=SCREEN_W-10;
        for(int g=8;g>=0;g--){
            if(!g_battle.controlGroups[g].active) continue;
            bx-=20;
            bool isActive=(g_battle.activeControlGroup==g);
            Color bgc=isActive?Color{200,165,80,255}:Color{50,44,30,200};
            Color fgc=isActive?Color{10,5,0,255}:Color{180,160,90,255};
            DrawRectangle(bx,4,18,22,bgc);
            DrawRectangleLinesEx({(float)bx,4,18,22},1,{120,100,40,200});
            DrawText(TextFormat("%d",g+1),bx+5,7,12,fgc);
        }
    }

    // Bottom HUD
    DrawRectangle(0,hudY,SCREEN_W,HUD_H,{8,6,4,240});
    DrawLine(0,hudY,SCREEN_W,hudY,{80,65,30,200});

    // Selected unit portrait (left)
    BattleUnit* sel=nullptr;
    for(auto& bu:g_battle.playerUnits) if(bu.selected&&bu.typeIdx<unitTypeCount()){sel=&bu;break;}

    if(sel){
        const UnitTypeDef& td=g_unitTypes[sel->typeIdx];
        int alive=0; for(auto& s:sel->soldiers) if(s.alive) alive++;
        // Portrait box
        DrawRectangle(8,hudY+6,58,58,{td.r,td.g,td.b,100});
        DrawRectangleLinesEx({8,(float)(hudY+6),58,58},2,C_GOLD);
        drawUnit(sel->typeIdx,0,{37.f,(float)(hudY+61)},
                 g_menuTime*30.f,1.15f,true,false,UA_IDLE,g_menuTime);
        // Info
        DrawText(td.name,72,hudY+8,15,C_PARCHMENT);
        // HP bar
        float total=(float)(td.soldierCount)*td.hpPerSoldier;
        float curHp=0.f; for(auto& s:sel->soldiers) if(s.alive) curHp+=s.hp;
        float hpRat=curHp/total;
        DrawRectangle(72,hudY+28,220,10,DARKGRAY);
        DrawRectangle(72,hudY+28,(int)(220*hpRat),10,hpRat>0.5f?Color{0,228,48,255}:hpRat>0.25f?Color{253,249,0,255}:Color{230,41,55,255});
        // Morale bar
        DrawRectangle(72,hudY+42,220,8,DARKGRAY);
        float mr=sel->morale/100.f;
        DrawRectangle(72,hudY+42,(int)(220*mr),8,mr>0.5f?Color{0,158,47,255}:mr>0.2f?Color{253,249,0,255}:Color{230,41,55,255});
        DrawText(TextFormat("Morale: %.0f%%",sel->morale),298,hudY+41,12,C_SECONDARY);
        DrawText(TextFormat("Soldiers: %d / %d",alive,td.soldierCount),72,hudY+54,12,C_SECONDARY);
    } else {
        DrawText("No unit selected",12,hudY+34,14,{70,70,70,255});
    }

    // Center: group state and current orders
    if(sel){
        static const char* gsNames[]={"Idle","Advancing","Engaged","Routing"};
        const char* gsn=gsNames[(int)sel->groupState];
        Color gsc=sel->groupState==UGS_ENGAGED?C_ENEMY_COL:
                  sel->groupState==UGS_ROUTING?Color{230,41,55,255}:
                  sel->groupState==UGS_ADVANCING?C_ALLY:C_SECONDARY;
        int tw=MeasureText(gsn,16);
        DrawText(gsn,SCREEN_W/2-tw/2,hudY+25,16,gsc);

        // Show current orders
        if(sel->hasExplicitOrder){
            if(sel->orderAttack>=0&&sel->orderAttack<(int)g_battle.enemyUnits.size()){
                DrawText("📋 ATTACKING ENEMY UNIT",SCREEN_W/2-115,hudY+48,11,{255,100,100,255});
            } else if(sel->orderType==1){
                DrawText("📍 MOVING TO POSITION",SCREEN_W/2-110,hudY+48,11,{100,180,255,255});
            }
        } else {
            DrawText("🎯 IDLE (AUTO)",SCREEN_W/2-60,hudY+48,11,C_SECONDARY);
        }
    }

    // Right: counters + pause button
    int pa=0,ea=0;
    for(auto& bu:g_battle.playerUnits) for(auto& s:bu.soldiers) if(s.alive) pa++;
    for(auto& bu:g_battle.enemyUnits) for(auto& s:bu.soldiers) if(s.alive) ea++;
    DrawText(TextFormat("Allies: %d",pa),(int)(SCREEN_W-320),hudY+14,14,{80,160,255,255});
    DrawText(TextFormat("Enemies: %d",ea),(int)(SCREEN_W-320),hudY+34,14,C_ENEMY_COL);

    if(drawSmBtn({(float)(SCREEN_W-120),(float)(hudY+8),110,36},"PAUSE",mouse,{40,35,20,255},{70,60,35,255}))
        g_battle.paused=true;
    if(drawSmBtn({(float)(SCREEN_W-120),(float)(hudY+50),110,28},
                 g_battle.timeScale>1.f?"Speed: x2":"Speed: x1",mouse,
                 {30,30,50,255},{50,50,80,255}))
        g_battle.timeScale=(g_battle.timeScale>1.f)?1.f:2.f;

    // 3.6: Minimap (150x90px, bottom-right above HUD)
    int mmW=150,mmH=90;
    int mmX=SCREEN_W-160, mmY=hudY-mmH-8;
    DrawRectangle(mmX,mmY,mmW,mmH,{0,0,0,180});
    DrawRectangleLinesEx({(float)mmX,(float)mmY,(float)mmW,(float)mmH},1,{80,65,30,200});
    // Draw unit dots
    for(auto& bu:g_battle.playerUnits){
        int ac=0; for(auto& s:bu.soldiers) if(s.alive) ac++;
        if(ac==0) continue;
        int mx2=(int)(mmX+bu.anchorPos.x/(float)BATTLE_W*mmW);
        int my2=(int)(mmY+bu.anchorPos.y/(float)BATTLE_H*mmH);
        DrawRectangle(mx2-2,my2-2,4,4,C_ALLY);
    }
    for(auto& bu:g_battle.enemyUnits){
        int ac=0; for(auto& s:bu.soldiers) if(s.alive) ac++;
        if(ac==0) continue;
        int mx2=(int)(mmX+bu.anchorPos.x/(float)BATTLE_W*mmW);
        int my2=(int)(mmY+bu.anchorPos.y/(float)BATTLE_H*mmH);
        DrawRectangle(mx2-2,my2-2,4,4,C_ENEMY_COL);
    }
    // Draw viewport rect on minimap
    float vx,vy,vw,vh;
    battleViewRect(vx,vy,vw,vh);
    float vwW=vw/(float)BATTLE_W*mmW;
    float vhR=vh/(float)BATTLE_H*mmH;
    float vwX=mmX+vx/(float)BATTLE_W*mmW;
    float vwY=mmY+vy/(float)BATTLE_H*mmH;
    DrawRectangleLinesEx({vwX,vwY,vwW,vhR},1,WHITE);

    // 6.4: Battle log (last 5 entries, fade older ones)
    int logX=8, logY=hudY-8;
    int logCount=std::min(5,(int)g_battleLog.size());
    for(int i=0;i<logCount;i++){
        unsigned char a=(unsigned char)(200-i*35);
        DrawText(g_battleLog[i].c_str(),logX,logY-(i+1)*16,10,{180,160,120,a});
    }

    // 4.9: FPS counter
    if(g_settings.showFPS) DrawFPS(8,8);
}

// ═══════════════════════════════════════════════════════════════════════════
