/* Scripted keyboard input for host tests; DOS uses Open Watcom's conio.h. */
#include <stdio.h>
#define kbhit() 1
static int getch(void)
{
    int ch = getchar();
    return ch == EOF ? 26 : ch;
}
