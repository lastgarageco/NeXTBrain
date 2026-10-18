#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "Devices.h"
#include "MacTCP.h"
#include "../network.h"
#include "../wire.h"

static TCPiopb *pending;
static TCPiopb *abortPending;
static unsigned long ticks;
static short submitError;
static char sent[512];
static short sentLength;

unsigned long TickCount(void) { return ticks; }
OSErr OpenDriver(const unsigned char *name, short *reference)
{
    assert(name[0] == 4 && memcmp(name + 1, ".IPP", 4) == 0);
    *reference = 9;
    return noErr;
}
OSErr PBControl(ParmBlkPtr block, Boolean async)
{
    TCPiopb *pb = (TCPiopb *)block;
    wdsEntry *list;
    assert(async && pb->ioCRefNum == 9);
    if (submitError)
    {
        short error = submitError;
        submitError = 0;
        return error;
    }
    if (pb->csCode == TCPAbort)
        abortPending = pb;
    else
        pending = pb;
    if (pb->csCode == TCPSend)
    {
        list = (wdsEntry *)pb->csParam.send.wdsPtr;
        assert(list[1].length == 0 && pb->csParam.send.pushFlag);
        sentLength = list[0].length;
        memcpy(sent, list[0].ptr, sentLength);
    }
    return noErr;
}

static void Connect(void)
{
    assert(NetConnect(0x7f000001UL, 5555));
    assert(pending->csCode == TCPCreate);
    assert(pending->csParam.create.rcvBuffLen >= 4096);
    assert(NetPoll() == netNone); /* Still pending, no reuse. */
    pending->tcpStream = 123;
    pending->ioResult = noErr;
    assert(NetPoll() == netNone);
    assert(pending->csCode == TCPActiveOpen);
    assert(pending->csParam.open.remoteHost == 0x7f000001UL);
    assert(pending->csParam.open.remotePort == 5555);
    pending->ioResult = noErr;
    assert(NetPoll() == netConnected && NetReady());
}

static void Send(const char *text, Boolean clear)
{
    assert(NetSend(text, (short)strlen(text), clear));
    assert(!NetSend(text, (short)strlen(text), clear));
    assert(sentLength == (short)strlen(text));
    assert(memcmp(sent, text, sentLength) == 0);
    pending->ioResult = noErr;
    assert(NetPoll() == netNone);
    assert(pending->csCode == TCPRcv);
}

static int Receive(const char *text)
{
    assert(pending->csCode == TCPRcv);
    memcpy(pending->csParam.receive.rcvBuff, text, strlen(text));
    pending->csParam.receive.rcvBuffLen = (unsigned short)strlen(text);
    pending->ioResult = noErr;
    return NetPoll();
}

static void Disconnect(int expected)
{
    short command = pending->csCode;
    NetDisconnect();
    assert(NetPoll() == netNone);
    assert(abortPending->csCode == TCPAbort);
    abortPending->ioResult = noErr;
    /* An outstanding receive must finish before TCPRelease reuses its PB. */
    if (pending->ioResult > 0)
    {
        assert(NetPoll() == netNone && pending->csCode == command);
        pending->ioResult = -23012;
    }
    assert(NetPoll() == netNone && pending->csCode == TCPRelease);
    pending->ioResult = noErr;
    assert(NetPoll() == expected && NetOff());
}

int main(void)
{
    unsigned long ip;
    unsigned short port;
    char text[512];
    assert(ParseAddress("192.0.2.10", "5555", &ip, &port));
    assert(ip == 0xc000020aUL && port == 5555);
    assert(!ParseAddress("0.0.0.0", "5555", &ip, &port));
    assert(!ParseAddress("256.1.2.3", "5555", &ip, &port));
    assert(!ParseAddress("1.2.3", "5555", &ip, &port));
    assert(!ParseAddress("1.2.3.4", "65536", &ip, &port));
    assert(!ParseAddress("1.2.3.4", "55x", &ip, &port));
    assert(MakeQuestion("A\rB", 3, text) == 4 && memcmp(text, "A B\n", 4) == 0);
    assert(!MakeQuestion(" \t", 2, text));
    assert(!MakeQuestion("\200", 1, text));
    assert(!MakeQuestion("", 511, text));
    strcpy(text, "A\n\303\251\r");
    assert(DisplayText(text, 5) == 4 && memcmp(text, "A\r??", 4) == 0);

    assert(NetInit() == noErr);
    Connect();
    Send("Question\n", false);
    assert(Receive("First line\n") == netData);
    assert(netDataLength == 11 && memcmp(netDataBuffer, "First line\n", 11) == 0);
    assert(NetPoll() == netNone);
    assert(Receive("Last line\n") == netData);
    NetPoll();
    pending->ioResult = commandTimeout;
    assert(NetPoll() == netFinished && NetReady());
    Send("EOT\n", true);
    assert(Receive("O") == netNone);
    assert(Receive("K") == netNone);
    assert(Receive("\n") == netCleared && NetReady());
    Send("Next question\n", false);
    Disconnect(netDisconnected);

    Connect();
    Send("EOT\n", true);
    assert(Receive("NO\n") == netNone);
    Disconnect(netError);

    Connect();
    Send("Slow\n", false);
    ticks += 180UL * 60;
    pending->ioResult = commandTimeout;
    assert(NetPoll() == netNone);
    Disconnect(netError);

    /* Immediate asynchronous submission failure must not leave us pending. */
    submitError = -50;
    assert(NetConnect(0x7f000001UL, 5555));
    assert(NetPoll() == netError && NetOff());

    /* Cancel during creation: wait for a stream before aborting it. */
    assert(NetConnect(0x7f000001UL, 5555));
    NetDisconnect();
    assert(NetPoll() == netNone && pending->csCode == TCPCreate);
    pending->tcpStream = 123;
    pending->ioResult = noErr;
    NetPoll();
    Disconnect(netDisconnected);

    /* Release failures must not let Quit free driver-owned storage. */
    Connect();
    NetDisconnect();
    NetPoll();
    abortPending->ioResult = noErr;
    NetPoll();
    assert(pending->csCode == TCPRelease);
    pending->ioResult = -50;
    assert(NetPoll() == netError && !NetOff());
    ticks += 60;
    assert(NetPoll() == netError && pending->ioResult > 0);
    pending->ioResult = noErr;
    assert(NetPoll() == netError && NetOff());
    puts("PASS: wire formatting, MacTCP state flow, split replies/ACK, timeouts, cancellation");
    return 0;
}
