#include "wire.h"

int ParseAddress(const char *host, const char *port,
                 unsigned long *address, unsigned short *number)
{
    unsigned long ip = 0;
    unsigned long value;
    int part;
    for (part = 0; part < 4; ++part)
    {
        value = 0;
        if (*host < '0' || *host > '9')
            return 0;
        do
        {
            value = value * 10 + (*host++ - '0');
            if (value > 255)
                return 0;
        } while (*host >= '0' && *host <= '9');
        ip = (ip << 8) | value;
        if (part < 3 && *host++ != '.')
            return 0;
    }
    if (*host || ip == 0 || ip == 0xffffffffUL)
        return 0;
    value = 0;
    if (!*port)
        return 0;
    while (*port)
    {
        if (*port < '0' || *port > '9')
            return 0;
        value = value * 10 + (*port++ - '0');
        if (value > 65535UL)
            return 0;
    }
    if (!value)
        return 0;
    *address = ip;
    *number = (unsigned short)value;
    return 1;
}

/* A request is one ASCII line, even when TextEdit contains pasted returns. */
int MakeQuestion(const char *text, int length, char *request)
{
    int i;
    int nonblank = 0;
    unsigned char ch;
    if (length <= 0 || length > 510)
        return 0;
    for (i = 0; i < length; ++i)
    {
        ch = (unsigned char)text[i];
        if (ch == '\r' || ch == '\n' || ch == '\t')
            ch = ' ';
        if (ch < 32 || ch > 126)
            return 0;
        if (ch != ' ')
            nonblank = 1;
        request[i] = (char)ch;
    }
    if (!nonblank)
        return 0;
    request[length] = '\n';
    return length + 1;
}

int DisplayText(char *text, int length)
{
    int in;
    int out = 0;
    unsigned char ch;
    for (in = 0; in < length; ++in)
    {
        ch = (unsigned char)text[in];
        if (ch == '\r')
            continue;
        if (ch == '\n')
            ch = '\r';
        else if (ch != '\t' && (ch < 32 || ch > 126))
            ch = '?';
        text[out++] = (char)ch;
    }
    return out;
}
