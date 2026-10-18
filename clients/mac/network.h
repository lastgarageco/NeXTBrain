#ifndef NB_NETWORK_H
#define NB_NETWORK_H
#include <Types.h>

enum { netNone, netConnected, netData, netFinished, netCleared,
       netDisconnected, netError };

OSErr NetInit(void);
int NetConnect(unsigned long host, unsigned short port);
int NetSend(const char *request, short length, Boolean clear);
void NetDisconnect(void);
int NetPoll(void);
Boolean NetOff(void);
Boolean NetReady(void);
Boolean NetClosing(void);
extern OSErr netErrorCode;
extern char netDataBuffer[512];
extern short netDataLength;

#endif
