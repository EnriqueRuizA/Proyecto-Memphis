// src/prof.cpp — Implementacion del profiler por secciones (Fase A)
#include "prof.h"
#include "config.h"
#include <cstdio>
#include <cstring>

bool g_profOverlay=false;
int g_profSoldiers=0;
double g_profFrame[PROF_COUNT]={};
double g_profAvg[PROF_COUNT]={};
static long long g_profFrames=0;

void profAdd(int sec,double ms){
    if(sec>=0&&sec<PROF_COUNT) g_profFrame[sec]+=ms;
}

void profFrameBegin(){
    memset(g_profFrame,0,sizeof(g_profFrame));
    g_profSoldiers=0; // solo lo rellena el update de batalla; menu = 0
}

void profFrameEnd(){
    g_profFrames++;
    // Media exponencial: alpha 0.05 ~ ventana de 0.3s a 60 fps
    for(int i=0;i<PROF_COUNT;i++)
        g_profAvg[i]=g_profAvg[i]*0.95+g_profFrame[i]*0.05;
    // Log cada ~2 s (120 frames): una linea por ventana, para comparativas
    if(g_profFrames%120==0){
        FILE* fp=fopen("build/prof_log.txt","a");
        if(fp){
            fprintf(fp,"fps=%.1f frame=%.2f input=%.2f update=%.2f sep=%.2f draw=%.2f terr=%.2f units=%.2f map=%.2f sol=%d\n",
                    1000.0/(g_profAvg[PROF_FRAME]>0.01?g_profAvg[PROF_FRAME]:16.7),
                    g_profAvg[PROF_FRAME],g_profAvg[PROF_INPUT],g_profAvg[PROF_UPDATE],
                    g_profAvg[PROF_SEP],g_profAvg[PROF_DRAW],g_profAvg[PROF_TERRAIN],
                    g_profAvg[PROF_UNITS],g_profAvg[PROF_MAP],g_profSoldiers);
            fclose(fp);
        }
    }
}

void profDrawOverlay(){
    static const char* names[PROF_COUNT]={"frame","input","update","sep","draw","terrain","units","map"};
    const int fs=14;
    int w=250, x=SCREEN_W-10-w, y=40;
    DrawRectangle(x-6,y-4,w+6,(PROF_COUNT+1)*18+8,{0,0,0,190});
    DrawText("F10 profiler (ms/frame)",x,y,fs,C_GOLD);
    y+=18;
    for(int i=0;i<PROF_COUNT;i++){
        char buf[64];
        if(i==PROF_MAP&&g_profAvg[i]<0.005&&g_profFrame[i]<0.005)
            snprintf(buf,63,"%-7s -",names[i]);
        else
            snprintf(buf,63,"%-7s %6.2f  (avg %5.2f)",names[i],g_profFrame[i],g_profAvg[i]);
        DrawText(buf,x,y,fs,C_PARCHMENT);
        y+=18;
    }
}
