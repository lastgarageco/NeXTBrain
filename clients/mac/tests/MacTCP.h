#ifndef TEST_MACTCP_H
#define TEST_MACTCP_H
#include "Types.h"
typedef unsigned long StreamPtr;
typedef struct { unsigned short length; Ptr ptr; } wdsEntry;
enum { TCPCreate = 30, TCPActiveOpen = 32, TCPSend = 34, TCPRcv = 36,
       TCPAbort = 40, TCPRelease = 42, commandTimeout = -23016 };
typedef struct {
    short ioCRefNum, csCode;
    volatile short ioResult;
    StreamPtr tcpStream;
    union {
        struct { Ptr rcvBuff; unsigned long rcvBuffLen; } create;
        struct {
            unsigned long remoteHost;
            unsigned short remotePort;
            char commandTimeoutValue, ulpTimeoutValue, ulpTimeoutAction, validityFlags;
        } open;
        struct {
            Ptr wdsPtr;
            Boolean pushFlag;
            char ulpTimeoutValue, ulpTimeoutAction, validityFlags;
        } send;
        struct { char commandTimeoutValue; Ptr rcvBuff; unsigned short rcvBuffLen; } receive;
    } csParam;
} TCPiopb;
#endif
