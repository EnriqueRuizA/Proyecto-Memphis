// src/net.cpp — Base de red UDP (Fase E)
// Winsock2 se incluye PRIMERO (antes de raylib/windows.h) para evitar el
// conflicto clasico winsock2.h despues de windows.h.
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#endif
#include "net.h"
#include <cstdio>
#include <cstring>

static bool g_netReady=false;

#ifdef _WIN32
typedef SOCKET netsock_t;
#else
typedef int netsock_t;
#endif

static void sockClose(NetSock s){
    if(s==NET_INVALID) return;
#ifdef _WIN32
    closesocket((SOCKET)s);
#else
    close((int)s);
#endif
}

bool netInit(){
    if(g_netReady) return true;
#ifdef _WIN32
    WSADATA wsa;
    if(WSAStartup(MAKEWORD(2,2),&wsa)!=0) return false;
#endif
    g_netReady=true;
    return true;
}

void netShutdown(){
    if(!g_netReady) return;
    g_netReady=false;
#ifdef _WIN32
    WSACleanup();
#endif
}

NetSock netOpen(unsigned short bindPort, unsigned short* outPort){
    if(!netInit()) return NET_INVALID;
    NetSock s=(NetSock)socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if(s==NET_INVALID) return NET_INVALID;
    // No bloqueante
#ifdef _WIN32
    u_long nb=1;
    if(ioctlsocket((SOCKET)s,FIONBIO,&nb)!=0){ sockClose(s); return NET_INVALID; }
#else
    int fl=fcntl((int)s,F_GETFL,0);
    if(fl<0 || fcntl((int)s,F_SETFL,fl|O_NONBLOCK)<0){ sockClose(s); return NET_INVALID; }
#endif
    sockaddr_in a;
    memset(&a,0,sizeof a);
    a.sin_family=AF_INET;
    a.sin_port=htons(bindPort);
    a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    if(bind(s,(struct sockaddr*)&a,sizeof a)!=0){ sockClose(s); return NET_INVALID; }
    if(outPort){
        sockaddr_in la;
        memset(&la,0,sizeof la);
#ifdef _WIN32
        int sl=(int)sizeof la;
#else
        socklen_t sl=(socklen_t)sizeof la;
#endif
        if(getsockname(s,(struct sockaddr*)&la,&sl)==0) *outPort=ntohs(la.sin_port);
        else *outPort=bindPort;
    }
    return s;
}

void netClose(NetSock s){ sockClose(s); }

int netSendTo(NetSock s, const char* ip, unsigned short port,
              const void* data, int len){
    if(s==NET_INVALID||!g_netReady) return -1;
    sockaddr_in d;
    memset(&d,0,sizeof d);
    d.sin_family=AF_INET;
    d.sin_port=htons(port);
    if(inet_pton(AF_INET,ip,&d.sin_addr)!=1) return -1;
    int r=(int)sendto((netsock_t)s,(const char*)data,len,0,(struct sockaddr*)&d,sizeof d);
    return r;
}

int netRecvFrom(NetSock s, void* buf, int maxLen, int timeoutMs){
    if(s==NET_INVALID||!g_netReady||maxLen<=0) return -1;
    fd_set rf;
    FD_ZERO(&rf);
#ifdef _WIN32
    FD_SET((SOCKET)s,&rf);
#else
    FD_SET((int)s,&rf);
#endif
    timeval tv;
    tv.tv_sec=timeoutMs/1000;
    tv.tv_usec=(timeoutMs%1000)*1000;
#ifdef _WIN32
    int r=select(0,&rf,NULL,NULL,&tv);
#else
    int r=select((int)s+1,&rf,NULL,NULL,&tv);
#endif
    if(r==0) return 0;   // timeout
    if(r<0) return -1;   // error
    sockaddr_in from;
    memset(&from,0,sizeof from);
#ifdef _WIN32
    int sl=(int)sizeof from;
#else
    socklen_t sl=(socklen_t)sizeof from;
#endif
    int n=(int)recvfrom((netsock_t)s,(char*)buf,maxLen,0,(struct sockaddr*)&from,&sl);
    return n;
}

int runNetTest(){
    const int ROUNDS=10;
    printf("NETTEST: UDP loopback 127.0.0.1:7777\n");
    if(!netInit()){
        printf("NETTEST FAIL - netInit (WSAStartup)\n");
        return 1;
    }
    unsigned short srvPort=0, cliPort=0;
    NetSock srv=netOpen(7777,&srvPort);
    if(srv==NET_INVALID){
        printf("NETTEST FAIL - bind 127.0.0.1:7777 (puerto ocupado?)\n");
        netShutdown();
        return 1;
    }
    NetSock cli=netOpen(0,&cliPort);
    if(cli==NET_INVALID){
        printf("NETTEST FAIL - socket cliente\n");
        netClose(srv);
        netShutdown();
        return 1;
    }
    printf("NETTEST: servidor :%u cliente :%u\n",
           (unsigned)srvPort,(unsigned)cliPort);
    int ok=0;
    for(int i=0;i<ROUNDS;i++){
        char msg[64];
        int mlen=snprintf(msg,sizeof msg,"PING %d",i);
        if(mlen<=0||netSendTo(cli,"127.0.0.1",7777,msg,mlen)!=mlen){
            printf("NETTEST round %d: send fallo\n",i);
            continue;
        }
        char buf[256];
        int rl=netRecvFrom(srv,buf,sizeof buf-1,500);
        if(rl<=0){
            printf("NETTEST round %d: servidor sin datagrama (recv=%d)\n",i,rl);
            continue;
        }
        buf[rl]='\0';
        if(netSendTo(srv,"127.0.0.1",cliPort,buf,rl)!=rl){
            printf("NETTEST round %d: eco fallo\n",i);
            continue;
        }
        rl=netRecvFrom(cli,buf,sizeof buf-1,500);
        if(rl<=0){
            printf("NETTEST round %d: cliente sin eco (recv=%d)\n",i,rl);
            continue;
        }
        buf[rl]='\0';
        if(strncmp(buf,"PING",4)==0){
            ok++;
            printf("NETTEST round %d: OK (%d bytes)\n",i,rl);
        }else{
            printf("NETTEST round %d: payload inesperado '%s'\n",i,buf);
        }
    }
    netClose(cli);
    netClose(srv);
    netShutdown();
    if(ok==ROUNDS){
        printf("NETTEST PASS (%d/%d)\n",ok,ROUNDS);
        return 0;
    }
    printf("NETTEST FAIL (%d/%d)\n",ok,ROUNDS);
    return 1;
}
