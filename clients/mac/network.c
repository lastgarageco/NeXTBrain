/* Direct asynchronous MacTCP calls. No BSD sockets or completion callbacks. */
#include <Devices.h>
#include <MacTCP.h>
#include <OSUtils.h>
#include <string.h>
#include "network.h"

enum { off, creating, opening, ready, sending, receiving, aborting, releasing };
static short state = off;
static short driver;
static StreamPtr stream;
static TCPiopb operation;
static TCPiopb abortOperation;
static char receiveStorage[8192];
static char sendStorage[512];
static wdsEntry writeList[2];
static unsigned long remoteHost;
static unsigned short remotePort;
static unsigned long lastData;
static Boolean cancel;
static Boolean clearing;
static Boolean failed;
static unsigned long releaseRetry;
static short ackLength;
static char lastByte;
OSErr netErrorCode;
char netDataBuffer[512];
short netDataLength;

static void Prepare(TCPiopb *pb, short command)
{
    memset(pb, 0, sizeof(*pb));
    pb->ioCRefNum = driver;
    pb->tcpStream = stream;
    pb->csCode = command;
}

static void Submit(TCPiopb *pb)
{
    OSErr error;
    pb->ioResult = 1;
    error = PBControl((ParmBlkPtr)pb, true);
    if (error != noErr)
        pb->ioResult = error;
}

/* Device Manager changes ioResult asynchronously; force a fresh read. */
static OSErr Result(TCPiopb *pb)
{
    return ((volatile TCPiopb *)pb)->ioResult;
}

OSErr NetInit(void)
{
    static unsigned char name[] = {4, '.', 'I', 'P', 'P'};
    return OpenDriver(name, &driver);
}

Boolean NetOff(void) { return state == off; }
Boolean NetReady(void) { return state == ready; }
Boolean NetClosing(void) { return cancel || state == aborting || state == releasing; }

int NetConnect(unsigned long host, unsigned short port)
{
    if (state != off)
        return 0;
    remoteHost = host;
    remotePort = port;
    cancel = failed = false;
    netErrorCode = noErr;
    Prepare(&operation, TCPCreate);
    operation.csParam.create.rcvBuff = receiveStorage;
    operation.csParam.create.rcvBuffLen = sizeof(receiveStorage);
    state = creating;
    Submit(&operation);
    return 1;
}

int NetSend(const char *request, short length, Boolean clear)
{
    if (state != ready || length < 1 || length > (short)sizeof(sendStorage))
        return 0;
    memcpy(sendStorage, request, length);
    writeList[0].length = length;
    writeList[0].ptr = sendStorage;
    writeList[1].length = 0;
    writeList[1].ptr = NULL;
    clearing = clear;
    ackLength = 0;
    lastByte = 0;
    Prepare(&operation, TCPSend);
    operation.csParam.send.wdsPtr = (Ptr)writeList;
    operation.csParam.send.pushFlag = true;
    operation.csParam.send.ulpTimeoutValue = 30;
    operation.csParam.send.ulpTimeoutAction = 1;
    operation.csParam.send.validityFlags = (char)0xc0;
    state = sending;
    Submit(&operation);
    return 1;
}

void NetDisconnect(void)
{
    if (state != off)
        cancel = true;
}

static void Fail(OSErr error)
{
    netErrorCode = error;
    failed = true;
    cancel = true;
}

static void Receive(void)
{
    Prepare(&operation, TCPRcv);
    operation.csParam.receive.commandTimeoutValue = 1;
    operation.csParam.receive.rcvBuff = netDataBuffer;
    operation.csParam.receive.rcvBuffLen = sizeof(netDataBuffer);
    state = receiving;
    Submit(&operation);
}

int NetPoll(void)
{
    OSErr result;
    short i;
    int event = netNone;

    if (state == off)
        return netNone;
    /* Creation must finish before a stream exists to abort. */
    if (cancel && state != creating && state != aborting && state != releasing)
    {
        Prepare(&abortOperation, TCPAbort);
        state = aborting;
        Submit(&abortOperation);
    }
    if (state == aborting)
    {
        /* Never reuse a parameter block/buffer while the driver owns it. */
        if (Result(&abortOperation) > 0 || Result(&operation) > 0)
            return netNone;
        Prepare(&operation, TCPRelease);
        state = releasing;
        releaseRetry = TickCount();
        Submit(&operation);
        return netNone;
    }
    result = Result(&operation);
    if (result > 0)
        return netNone;
    if (state == releasing)
    {
        if (result != noErr)
        {
            /* Keep the app/buffers alive and retry local cleanup, even on Quit. */
            failed = true;
            netErrorCode = result;
            if ((unsigned long)(TickCount() - releaseRetry) >= 60)
            {
                Prepare(&operation, TCPRelease);
                releaseRetry = TickCount();
                Submit(&operation);
            }
            return netError;
        }
        stream = 0;
        state = off;
        cancel = false;
        return failed ? netError : netDisconnected;
    }
    if (state == creating)
    {
        if (result != noErr)
        {
            state = off;
            cancel = false;
            netErrorCode = result;
            return netError;
        }
        stream = operation.tcpStream;
        Prepare(&operation, TCPActiveOpen);
        operation.csParam.open.remoteHost = remoteHost;
        operation.csParam.open.remotePort = remotePort;
        operation.csParam.open.commandTimeoutValue = 30;
        operation.csParam.open.ulpTimeoutValue = 30;
        operation.csParam.open.ulpTimeoutAction = 1;
        operation.csParam.open.validityFlags = (char)0xc0;
        state = opening;
        if (cancel)
            operation.ioResult = noErr;
        else
            Submit(&operation);
        return netNone;
    }
    if (state == ready)
        return netNone;
    if (state == receiving && result == commandTimeout)
    {
        if (!clearing && lastByte == '\n')
        {
            state = ready;
            return netFinished;
        }
        if ((unsigned long)(TickCount() - lastData) >= 180UL * 60)
            Fail(commandTimeout);
        else
            Receive();
        return netNone;
    }
    if (result != noErr)
    {
        Fail(result);
        return netNone;
    }
    if (state == opening)
    {
        state = ready;
        return netConnected;
    }
    if (state == sending)
    {
        lastData = TickCount();
        Receive();
        return netNone;
    }
    if (state == receiving)
    {
        netDataLength = operation.csParam.receive.rcvBuffLen;
        if (netDataLength <= 0 || netDataLength > (short)sizeof(netDataBuffer))
        {
            Fail(paramErr);
            return netNone;
        }
        lastData = TickCount();
        lastByte = netDataBuffer[netDataLength - 1];
        if (clearing)
        {
            for (i = 0; i < netDataLength; ++i)
            {
                if (ackLength >= 3 || netDataBuffer[i] != "OK\n"[ackLength++])
                {
                    Fail(paramErr);
                    return netNone;
                }
            }
            if (ackLength == 3)
            {
                state = ready;
                return netCleared;
            }
            Receive();
        }
        else
        {
            /* Return the data before starting another receive into this buffer. */
            state = sending;
            event = netData;
        }
    }
    return event;
}
