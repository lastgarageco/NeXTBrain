/* System 7.1, 68K Toolbox application. Compile with non-opaque interfaces. */
#include <Types.h>
#include <Quickdraw.h>
#include <Fonts.h>
#include <Windows.h>
#include <Menus.h>
#include <TextEdit.h>
#include <TextUtils.h>
#include <Dialogs.h>
#include <Events.h>
#include <Controls.h>
#include <Memory.h>
#include <OSUtils.h>
#include <Scrap.h>
#include <Desk.h>
#include <string.h>
#include "network.h"
#include "wire.h"

static WindowPtr window;
static TEHandle fields[4]; /* address, port, question, read-only transcript */
static short focus = 2;
static Rect boxes[4] = {
    {12, 48, 28, 202}, {12, 246, 28, 310},
    {226, 14, 248, 350}, {72, 14, 202, 448}
};
static Rect statusBox = {258, 12, 282, 356};
static ControlHandle connectButton, sendButton, clearButton, scrollBar;
static short scrollLine;
static char statusText[128] = "Enter server address, then Connect.";
static Boolean quitting;
static Boolean front = true;
static Boolean driverOK;

static unsigned char *Pascal(const char *text, unsigned char *out)
{
    short length = (short)strlen(text);
    if (length > 255)
        length = 255;
    out[0] = (unsigned char)length;
    memcpy(out + 1, text, length);
    return out;
}

static void Status(const char *text)
{
    strncpy(statusText, text, sizeof(statusText) - 1);
    statusText[sizeof(statusText) - 1] = 0;
    InvalRect(&statusBox);
}

static void SetFocus(short field)
{
    TEDeactivate(fields[focus]);
    focus = field;
    if (front)
        TEActivate(fields[focus]);
}

static short LastLine(void)
{
    short visible = (boxes[3].bottom - boxes[3].top) / (*fields[3])->lineHeight;
    short last = (*fields[3])->nLines - visible + 1;
    return last > 0 ? last : 0;
}

static void ScrollTo(short line)
{
    if (line < 0)
        line = 0;
    if (line > LastLine())
        line = LastLine();
    TEScroll(0, (scrollLine - line) * (*fields[3])->lineHeight, fields[3]);
    scrollLine = line;
    SetControlMaximum(scrollBar, LastLine());
    SetControlValue(scrollBar, scrollLine);
}

static void Append(const char *text, short length)
{
    TEHandle transcript = fields[3];
    short remove;
    ScrollTo(0);
    /* Stay below classic TextEdit's 32K text and coordinate limits. */
    while ((*transcript)->teLength + length > 12000 || (*transcript)->nLines > 1000)
    {
        remove = (*transcript)->teLength / 2;
        if (remove == 0)
            break;
        TESetSelect(0, remove, transcript);
        TEDelete(transcript);
    }
    TESetSelect((*transcript)->teLength, (*transcript)->teLength, transcript);
    TEInsert(text, length, transcript);
    ScrollTo(LastLine());
}

static void ClearTranscript(void)
{
    ScrollTo(0);
    TESetText("", 0, fields[3]);
    ScrollTo(0);
}

static void TextOf(short field, char *out)
{
    short length = (*fields[field])->teLength;
    memcpy(out, *(*fields[field])->hText, length);
    out[length] = 0;
}

static void Connect(void)
{
    char host[64], port[8];
    unsigned long address;
    unsigned short number;
    if (!NetOff())
    {
        NetDisconnect();
        Status("Disconnecting...");
        return;
    }
    TextOf(0, host);
    TextOf(1, port);
    if (!ParseAddress(host, port, &address, &number))
    {
        Status("Enter an IPv4 address and port 1-65535.");
        return;
    }
    if (NetConnect(address, number))
    {
        SetFocus(2);
        Status("Connecting...");
    }
}

static void SendQuestion(void)
{
    char request[512];
    short length;
    length = MakeQuestion(*(*fields[2])->hText, (*fields[2])->teLength, request);
    if (!length)
    {
        Status("Use 1-510 ASCII characters for a question.");
        return;
    }
    if (NetSend(request, length, false))
    {
        Append("You: ", 5);
        request[length - 1] = '\r';
        Append(request, length);
        Append("NeXTBrain: ", 11);
        TESetText("", 0, fields[2]);
        Status("Waiting for NeXTBrain...");
    }
}

static void Buttons(void)
{
    unsigned char title[256];
    SetControlTitle(connectButton, Pascal(NetOff() ? "Connect" : "Disconnect", title));
    HiliteControl(connectButton, driverOK && !NetClosing() && !quitting ? 0 : 255);
    HiliteControl(sendButton, NetReady() && !quitting ? 0 : 255);
    HiliteControl(clearButton, (NetReady() || NetOff()) && !quitting ? 0 : 255);
}

static void NetworkEvent(void)
{
    int event = NetPoll();
    unsigned char number[256];
    char message[128];
    switch (event)
    {
        case netConnected:
            ClearTranscript();
            Status("Connected. Ready.");
            break;
        case netData:
            Append(netDataBuffer, DisplayText(netDataBuffer, netDataLength));
            Status("Receiving...");
            break;
        case netFinished:
            Append("\r", 1);
            Status("Ready.");
            break;
        case netCleared:
            ClearTranscript();
            Status("Conversation cleared. Ready.");
            break;
        case netDisconnected:
            Status("Disconnected.");
            break;
        case netError:
            NumToString(netErrorCode, number);
            strcpy(message, "MacTCP error ");
            strncat(message, (char *)number + 1, number[0]);
            Status(message);
            break;
    }
    Buttons();
}

static short Limit(void)
{
    if (focus == 0) return 63;
    if (focus == 1) return 5;
    return 510;
}

static Boolean Editable(void)
{
    return focus == 2 || (focus < 2 && NetOff());
}

static void MenuCommand(long choice)
{
    short menu = (short)(choice >> 16);
    short item = (short)choice;
    long offset;
    long size;
    short remaining;
    unsigned char name[256];
    if (menu == 127)
    {
        if (item == 1)
            Status("NeXTBrain - Classic Macintosh client");
        else if (item > 1)
        {
            GetMenuItemText(GetMenuHandle(127), item, name);
            OpenDeskAcc(name);
            SetPort(window);
        }
    }
    if (menu == 128 && item == 1)
    {
        quitting = true;
        NetDisconnect();
        Status("Disconnecting before quitting...");
    }
    if (menu == 129)
    {
        if (item == 2 || (item == 1 && Editable()))
        {
            TECopy(fields[focus]);
            ZeroScrap();
            TEToScrap();
            if (item == 1)
                TEDelete(fields[focus]);
        }
        if (item == 3 && Editable())
        {
            size = GetScrap(NULL, 'TEXT', &offset);
            remaining = (*fields[focus])->teLength -
                        ((*fields[focus])->selEnd - (*fields[focus])->selStart);
            if (size >= 0 && size <= Limit() - remaining)
            {
                TEFromScrap();
                TEPaste(fields[focus]);
            }
            else
                SysBeep(1);
        }
        if (item == 4)
            TESetSelect(0, (*fields[focus])->teLength, fields[focus]);
    }
    HiliteMenu(0);
}

static void Draw(void)
{
    short i;
    Rect frame;
    unsigned char text[256];
    EraseRect(&window->portRect);
    TextFont(0);
    TextSize(12);
    MoveTo(12, 25); DrawString(Pascal("Host", text));
    MoveTo(212, 25); DrawString(Pascal("Port", text));
    MoveTo(12, 62); DrawString(Pascal("Conversation", text));
    MoveTo(12, 219); DrawString(Pascal("Question", text));
    for (i = 0; i < 4; ++i)
    {
        frame = boxes[i];
        InsetRect(&frame, -2, -2);
        FrameRect(&frame);
        TEUpdate(&boxes[i], fields[i]);
    }
    TextFont(1);
    TextSize(9);
    MoveTo(12, 274); DrawString(Pascal(statusText, text));
    DrawControls(window);
}

static void Mouse(EventRecord *event)
{
    WindowPtr hitWindow;
    short part, i, oldValue, value;
    Point point = event->where;
    ControlHandle control;
    Rect drag = qd.screenBits.bounds;
    part = FindWindow(point, &hitWindow);
    if (part == inMenuBar)
        MenuCommand(MenuSelect(point));
    else if (part == inSysWindow)
        SystemClick(event, hitWindow);
    else if (hitWindow == window && part == inDrag)
        DragWindow(window, point, &drag);
    else if (hitWindow == window && part == inGoAway)
    {
        if (TrackGoAway(window, point))
            MenuCommand(((long)128 << 16) | 1);
    }
    else if (hitWindow == window && part == inContent)
    {
        if (FrontWindow() != window)
        {
            SelectWindow(window);
            return;
        }
        GlobalToLocal(&point);
        if (FindControl(point, window, &control))
        {
            oldValue = GetControlValue(control);
            part = TrackControl(control, point, NULL);
            if (control == scrollBar && part)
            {
                value = oldValue;
                if (part == inThumb) value = GetControlValue(control);
                if (part == inUpButton) --value;
                if (part == inDownButton) ++value;
                if (part == inPageUp) value -= 8;
                if (part == inPageDown) value += 8;
                ScrollTo(value);
            }
            else if (part && control == connectButton)
                Connect();
            else if (part && control == sendButton && NetReady())
                SendQuestion();
            else if (part && control == clearButton)
            {
                if (NetOff())
                    ClearTranscript();
                else if (NetSend("EOT\n", 4, true))
                    Status("Clearing conversation...");
            }
            Buttons();
            return;
        }
        for (i = 0; i < 4; ++i)
            if (PtInRect(point, &boxes[i]) && (i >= 2 || NetOff()))
            {
                SetFocus(i);
                TEClick(point, (event->modifiers & shiftKey) != 0, fields[i]);
                break;
            }
    }
}

static void Key(EventRecord *event)
{
    char ch = (char)(event->message & charCodeMask);
    if (event->modifiers & cmdKey)
        MenuCommand(MenuKey(ch));
    else if (ch == '\t')
        SetFocus(NetOff() ? (focus + 1) % 3 : 2);
    else if (Editable())
    {
        if (ch == '\r' && focus == 2 && NetReady())
            SendQuestion();
        else if (ch == '\b' || (ch >= 28 && ch <= 31) ||
                 ((unsigned char)ch >= 32 && ch != 127 &&
                  (*fields[focus])->teLength -
                  ((*fields[focus])->selEnd - (*fields[focus])->selStart) < Limit()))
            TEKey(ch, fields[focus]);
    }
}

static ControlHandle Button(const char *label, short left, short top, short right)
{
    Rect rect;
    unsigned char title[256];
    SetRect(&rect, left, top, right, top + 22);
    return NewControl(window, &rect, Pascal(label, title), true, 0, 0, 1, pushButProc, 0);
}

int main(void)
{
    Rect bounds = {40, 16, 330, 496};
    Rect scroll = {70, 452, 204, 468};
    EventRecord event;
    MenuHandle menu;
    unsigned char text[256];
    short i;
    MaxApplZone();
    InitGraf(&qd.thePort);
    InitFonts();
    InitWindows();
    InitMenus();
    TEInit();
    InitDialogs(NULL);
    InitCursor();
    menu = NewMenu(127, Pascal("\024", text));
    AppendMenu(menu, Pascal("About NeXTBrain...;(-", text));
    AddResMenu(menu, 'DRVR');
    InsertMenu(menu, 0);
    menu = NewMenu(128, Pascal("File", text));
    AppendMenu(menu, Pascal("Quit/Q", text));
    InsertMenu(menu, 0);
    menu = NewMenu(129, Pascal("Edit", text));
    AppendMenu(menu, Pascal("Cut/X;Copy/C;Paste/V;Select All/A", text));
    InsertMenu(menu, 0);
    DrawMenuBar();
    window = NewWindow(NULL, &bounds, Pascal("NeXTBrain", text), true,
                       documentProc, (WindowPtr)-1L, true, 0);
    if (!window)
        return 1;
    SetPort(window);
    TextFont(1);
    TextSize(9);
    for (i = 0; i < 4; ++i)
    {
        fields[i] = TENew(&boxes[i], &boxes[i]);
        if (!fields[i])
            return 1;
        if (i < 3)
            TEAutoView(true, fields[i]);
    }
    TESetText("5555", 4, fields[1]);
    connectButton = Button("Connect", 332, 8, 468);
    sendButton = Button("Send", 366, 226, 468);
    clearButton = Button("Clear", 366, 258, 468);
    scrollBar = NewControl(window, &scroll, Pascal("", text), true,
                           0, 0, 0, scrollBarProc, 0);
    if (!connectButton || !sendButton || !clearButton || !scrollBar)
        return 1;
    SetFocus(0);
    driverOK = NetInit() == noErr;
    if (!driverOK)
        Status("MacTCP unavailable. Install/configure MacTCP.");
    Buttons();
    while (!quitting || !NetOff())
    {
        WaitNextEvent(everyEvent, &event, 1, NULL);
        SetPort(window);
        switch (event.what)
        {
            case mouseDown: Mouse(&event); break;
            case keyDown:
            case autoKey: if (!quitting) Key(&event); break;
            case updateEvt:
                if ((WindowPtr)event.message == window)
                {
                    BeginUpdate(window);
                    Draw();
                    EndUpdate(window);
                }
                break;
            case activateEvt:
                if ((WindowPtr)event.message == window)
                {
                    front = (event.modifiers & activeFlag) != 0;
                    if (front) TEActivate(fields[focus]);
                    else TEDeactivate(fields[focus]);
                }
                break;
            case osEvt:
                if ((event.message >> 24) == suspendResumeMessage)
                {
                    front = (event.message & resumeFlag) != 0;
                    if (front) TEActivate(fields[focus]);
                    else TEDeactivate(fields[focus]);
                }
                break;
        }
        if (front)
            TEIdle(fields[focus]);
        NetworkEvent();
    }
    for (i = 0; i < 4; ++i)
        TEDispose(fields[i]);
    DisposeWindow(window);
    return 0;
}
