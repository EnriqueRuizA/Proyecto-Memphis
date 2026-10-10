// src/net.h — Base de red UDP (Fase E) + protocolo de sesión (Fase F)
// Fundamento de red: sockets UDP no bloqueantes en loopback, verificables
// con el flag de linea de comandos -nettest (headless, sale con 0/1).
// Fase F anade el mensaje de sesión NetMsg (JOIN/WELCOME/PING/PONG/BYE)
// y el handshake minimo host<->cliente verificable en loopback.
// Fases posteriores (G/H) anadiran lobby y sincronización de campaña.
#pragma once
#include <stdint.h>

// Manejador opaco de socket (uintptr para casar con SOCKET de Winsock).
typedef uintptr_t NetSock;
#define NET_INVALID ((NetSock)-1)

// Puerto y version de protocolo (Fase F).
// Cambiar NET_PROTO_VERSION invalida handshakes con builds antiguos (REJECT).
#define NET_PORT          7777
#define NET_PROTO_VERSION 1

// WSAStartup/WSAStartup-equivalente. Idempotente; true si OK.
bool netInit();
// Deshace netInit (WSACleanup). Idempotente.
void netShutdown();

// Crea un socket UDP no bloqueante ligado a 127.0.0.1:bindPort
// (0 = puerto efimero). outPort recibe el puerto real asignado (puede ser 0).
// NET_INVALID si falla. Fase E/F: solo loopback (evita avisos de firewall);
// G/H ampliaran a INADDR_ANY cuando haya protocolo de juego.
NetSock netOpen(unsigned short bindPort, unsigned short* outPort);
void netClose(NetSock s);

// Envia datagrama a ip:port. Devuelve bytes enviados o -1.
int netSendTo(NetSock s, const char* ip, unsigned short port,
              const void* data, int len);
// Recibe datagrama esperando hasta timeoutMs. >0 = bytes, 0 = timeout, -1 = error.
int netRecvFrom(NetSock s, void* buf, int maxLen, int timeoutMs);

// ── Fase F: protocolo de sesión ─────────────────────────────────────────────
// Magic 'MCNT' en little-endian (M=0x4D C=0x43 N=0x4E T=0x54).
#define NET_MAGIC 0x544E434Du

enum NetMsgType : unsigned char {
    NET_MSG_JOIN    = 1,  // cliente -> host: pide unirse
    NET_MSG_WELCOME = 2,  // host -> cliente: aceptado (payload: version aceptada)
    NET_MSG_REJECT  = 3,  // host -> cliente: rechazado (payload: razon corta)
    NET_MSG_PING    = 4,  // keepalive either way (payload: eco del seq)
    NET_MSG_PONG    = 5,  // respuesta a PING
    NET_MSG_BYE     = 6   // cierre ordenado
};

#pragma pack(push,1)
struct NetMsg {
    uint32_t      magic;    // NET_MAGIC
    unsigned char type;     // NetMsgType
    unsigned char version;  // NET_PROTO_VERSION del emisor
    uint16_t      seq;      // secuencia estrictamente creciente por emisor
    char          payload[64];   // payload corto (texto o datos tipados)
};
#pragma pack(pop)

// Envia NetMsg rellenando magic y version automaticamente. Devuelve true si OK.
bool netSendMsg(NetSock s, const char* ip, unsigned short port,
                const NetMsg& m);
// Recibe y valida NetMsg (magic + version). Devuelve 1 OK, 0 timeout, -1 invalido.
int netRecvMsg(NetSock s, NetMsg* out, int timeoutMs);

// Self-test capa cruda: 10 ping/pong UDP entre dos sockets en 127.0.0.1:7777.
// Imprime NETTEST PASS/FAIL por stdout. Devuelve 0 = PASS, 1 = FAIL.
int runNetTest();
// Self-test sesión (Fase F): JOIN->WELCOME, 3x PING/PONG con seq, BYE.
// Imprime SESSION PASS/FAIL. Devuelve 0 = PASS, 1 = FAIL.
int runSessionTest();
