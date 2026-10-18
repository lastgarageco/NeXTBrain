/* NeXTBrain DOS client: Open Watcom, Watt-32, C89. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>
#include <tcp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define CONFIG_FILE "NBCLIENT.CFG"
#define LINE_SIZE 512
#define REPLY_TIMEOUT 180
#define REPLY_GAP 1

static int command_is(const char *text, const char *command)
{
    while (*text && *command)
    {
        if (toupper((unsigned char)*text++) != *command++)
            return 0;
    }
    return *text == '\0' && *command == '\0';
}

static int load_configuration(struct sockaddr_in *address)
{
    FILE *file;
    char line[128];
    char *end;
    long port;
    int have_host = 0;
    int have_port = 0;

    memset(address, 0, sizeof(*address));
    address->sin_family = AF_INET;
    file = fopen(CONFIG_FILE, "r");
    if (file == NULL)
    {
        printf("Could not open %s.\n", CONFIG_FILE);
        return 0;
    }
    while (fgets(line, sizeof(line), file) != NULL)
    {
        if (strchr(line, '\n') == NULL && !feof(file))
            break;
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '#' || line[0] == '\0')
            continue;
        if (strncmp(line, "HOST=", 5) == 0)
        {
            address->sin_addr.s_addr = inet_addr(line + 5);
            have_host = address->sin_addr.s_addr != INADDR_NONE &&
                        address->sin_addr.s_addr != INADDR_ANY;
            if (!have_host)
                break;
        }
        else if (strncmp(line, "PORT=", 5) == 0)
        {
            port = strtol(line + 5, &end, 10);
            have_port = end != line + 5 && *end == '\0' &&
                        port >= 1 && port <= 65535L;
            if (!have_port)
                break;
            address->sin_port = htons((unsigned short)port);
        }
        else
            break;
    }
    if (!feof(file) || ferror(file) || !have_host || !have_port)
    {
        fclose(file);
        puts("Invalid configuration. Use HOST=IPv4 and PORT=1..65535.");
        return 0;
    }
    fclose(file);
    return 1;
}

/* Poll the keyboard so Watt-32 can service TCP while the user types. */
static int read_line(char *line)
{
    int length = 0;
    int ch;
    int extended = 0;

    for (;;)
    {
        tcp_tick(NULL);
        if (!kbhit())
            continue;
        ch = getch();
        if (extended)
        {
            extended = 0;
            continue;
        }
        if (ch == 0 || ch == 224)
        {
            extended = 1;
            continue;
        }
        if (ch == 3 || ch == 26)
            return 0;
        if (ch == '\r' || ch == '\n')
        {
            line[length] = '\0';
            puts("");
            return 1;
        }
        if (ch == '\b' && length > 0)
        {
            --length;
            fputs("\b \b", stdout);
        }
        else if (ch >= 32 && ch < 127)
        {
            if (length < LINE_SIZE - 2)
            {
                line[length++] = (char)ch;
                putchar(ch);
            }
            else
                putchar('\a');
        }
        fflush(stdout);
    }
}

static int send_line(int sock, char *line)
{
    int length = (int)strlen(line);
    int sent = 0;
    int count;

    line[length++] = '\n';
    while (sent < length)
    {
        count = send(sock, line + sent, length - sent, 0);
        if (count <= 0)
        {
            puts("Could not send request.");
            return 0;
        }
        sent += count;
    }
    return 1;
}

static int receive_reply(int sock, int acknowledgment)
{
    char buffer[1024];
    const char *expected = "OK\n";
    int ack_bytes = 0;
    int last = 0;
    int count;
    int ready;
    int i;
    fd_set readers;
    struct timeval timeout;

    for (;;)
    {
        FD_ZERO(&readers);
        FD_SET(sock, &readers);
        /* Multiline replies have no explicit end marker in this protocol. */
        timeout.tv_sec = !acknowledgment && last == '\n' ?
                         REPLY_GAP : REPLY_TIMEOUT;
        timeout.tv_usec = 0;
        ready = select(sock + 1, &readers, NULL, NULL, &timeout);
        if (ready == 0 && !acknowledgment && last == '\n')
        {
            puts("");
            return 1;
        }
        if (ready <= 0)
        {
            puts("Response timed out or socket failed.");
            return 0;
        }
        count = recv(sock, buffer, sizeof(buffer), 0);
        if (count <= 0)
        {
            puts("Server closed the connection or receive failed.");
            return 0;
        }
        if (acknowledgment)
        {
            for (i = 0; i < count; ++i)
            {
                if (ack_bytes >= 3 || buffer[i] != expected[ack_bytes++])
                {
                    puts("Unexpected conversation-end acknowledgment.");
                    return 0;
                }
            }
            if (ack_bytes == 3)
                return 1;
        }
        else
        {
            /* DOS consoles are not UTF-8 terminals; keep output ASCII. */
            for (i = 0; i < count; ++i)
            {
                unsigned char ch = (unsigned char)buffer[i];
                if (ch == '\n' || ch == '\t' || (ch >= 32 && ch < 127))
                    putchar(ch);
                else if (ch != '\r')
                    putchar('?');
            }
            fflush(stdout);
            last = buffer[count - 1];
        }
    }
}

int main(void)
{
    struct sockaddr_in address;
    char line[LINE_SIZE];
    int sock;
    int asking = 0;
    int ending;
    int result = 0;

    if (!load_configuration(&address))
        return 1;
    if (sock_init() != 0)
    {
        puts("Watt-32 initialization failed. Check packet driver and WATTCP.CFG.");
        return 1;
    }
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        puts("Could not create socket.");
        return 1;
    }
    if (connect(sock, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        puts("Could not connect to NeXTBrain.");
        closesocket(sock);
        return 1;
    }
    puts("Connected to NeXTBrain. Type HELP for commands.");
    for (;;)
    {
        fputs(asking ? "ASK> " : "> ", stdout);
        fflush(stdout);
        if (!read_line(line))
            break;
        if (line[0] == '\0')
            continue;
        ending = asking && (command_is(line, "EOT") || command_is(line, "EOL"));
        if (!asking)
        {
            if (command_is(line, "QUIT"))
                break;
            if (command_is(line, "HELP"))
            {
                puts("HELP   Show commands\nASK    Start conversation\n"
                     "STATUS Show server status\nQUIT   Disconnect\n"
                     "In ASK: EOT or EOL ends the conversation. Ctrl-Z exits.");
                continue;
            }
            if (command_is(line, "ASK"))
            {
                asking = 1;
                continue;
            }
            if (!command_is(line, "STATUS"))
            {
                puts("Unknown command. Type HELP.");
                continue;
            }
        }
        if (!send_line(sock, line) || !receive_reply(sock, ending))
        {
            result = 1;
            break;
        }
        if (ending)
            asking = 0;
    }
    closesocket(sock);
    return result;
}
