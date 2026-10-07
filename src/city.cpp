#include "city.h"
#include "config.h"
#include "util.h"
#include "ui.h"
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

const char* bldNames[BLD_COUNT]={
    "Market","Farm","Sawmill","Quarry","Smithy",
    "Barracks","Stable","Archery Range","Armoury Workshop",
    "Walls Lv.1","Walls Lv.2","Mage Tower","Temple","Port"
};
const int bldGoldCost[BLD_COUNT]={200,120,150,200,300,200,350,250,400,300,600,500,250,400};
const int bldWoodCost[BLD_COUNT]={0,0,0,0,0,60,100,80,120,0,0,0,0,100};
const int bldStoneCost[BLD_COUNT]={0,0,0,0,0,0,0,0,0,150,300,200,0,0};
const int bldIronCost[BLD_COUNT]={0,0,0,0,0,0,0,0,120,0,0,0,0,0};
const int bldTurns[BLD_COUNT]={2,1,2,3,3,2,3,2,4,3,4,5,2,3};
const int bldPrereq[BLD_COUNT]={-1,-1,1,-1,2,-1,5,5,4,-1,9,-1,-1,-1}; // Smithy req Sawmill, Stable/Range req Barracks, Workshop req Smithy, Walls2 req Walls1

// Gold/food/wood/stone/iron per turn per building
const float bldGoldYield[BLD_COUNT]={80,0,0,0,0,0,0,0,0,0,0,0,0,120};
const float bldFoodYield[BLD_COUNT]={0,60,0,0,0,0,0,0,0,0,0,0,0,0};
const float bldWoodYield[BLD_COUNT]={0,0,50,0,0,0,0,0,0,0,0,0,0,0};
const float bldStoneYield[BLD_COUNT]={0,0,0,40,0,0,0,0,0,0,0,0,0,0};
const float bldIronYield[BLD_COUNT]={0,0,0,0,30,0,0,0,0,0,0,0,0,0};
