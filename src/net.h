// src/net.h — Base de red UDP (Fase E)
// Fundamento de red: sockets UDP no bloqueantes en loopback, verificables
// con el flag de linea de comandos -nettest (headless, sale con 0/1).
// Fases posteriores (F/G/H) anadiran protocolo de juego; aqui solo la base.
#pragma once
#include <stdint.h>

// Manejador opaco de socket (uintptr para casar con SOCKET de Winsock).
typedef uintptr_t NetSock;
#define NET_INVALID ((NetSock)-1)

// WSAStartup/WSAStartup-equivalente. Idempotente; true si OK.
bool netInit();
// Deshace netInit (WSACleanup). Idempotente.
void netShutdown();

// Crea un socket UDP no bloqueante ligado a 127.0.0.1:bindPort
// (0 = puerto efimero). outPort recibe el puerto real asignado (puede ser 0).
// NET_INVALID si falla. Fase E: solo loopback (evita avisos de firewall);
// F/G/H ampliaran a INADDR_ANY cuando haya protocolo de juego.
NetSock netOpen(unsigned short bindPort, unsigned short* outPort);
void netClose(NetSock s);

// Envia datagrama a ip:port. Devuelve bytes enviados o -1.
int netSendTo(NetSock s, const char* ip, unsigned short port,
              const void* data, int len);
// Recibe datagrama esperando hasta timeoutMs. >0 = bytes, 0 = timeout, -1 = error.
int netRecvFrom(NetSock s, void* buf, int maxLen, int timeoutMs);

// Self-test: 10 ping/pong UDP entre dos sockets en 127.0.0.1:7777.
// Imprime NETTEST PASS/FAIL por stdout. Devuelve 0 = PASS, 1 = FAIL.
int runNetTest();
