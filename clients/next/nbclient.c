#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define CONFIG_FILE "nbclient.conf"

char server_host[64];
int server_port;


int load_configuration(void)
{
    FILE *file;
    char line[256];
    int host_found;
    int port_found;
    int length;

    host_found = 0;
    port_found = 0;

    file = fopen(CONFIG_FILE, "r");

    if (file == NULL)
    {
        printf("Configuration file '%s' not found.\n", CONFIG_FILE);
        return 0;
    }

    while (fgets(line, sizeof(line), file) != NULL)
    {
        length = strlen(line);

        if (length > 0 && line[length - 1] == '\n')
        {
            line[length - 1] = '\0';
        }

        if (line[0] == '#' || line[0] == '\0')
        {
            continue;
        }

        if (strncmp(line, "HOST=", 5) == 0)
        {
            strncpy(server_host, line + 5, sizeof(server_host) - 1);
            server_host[sizeof(server_host) - 1] = '\0';
            host_found = 1;
        }
        else if (strncmp(line, "PORT=", 5) == 0)
        {
            server_port = atoi(line + 5);
            port_found = 1;
        }
    }

    fclose(file);

    if (!host_found)
    {
        printf("HOST is missing from '%s'.\n", CONFIG_FILE);
        return 0;
    }

    if (!port_found)
    {
        printf("PORT is missing from '%s'.\n", CONFIG_FILE);
        return 0;
    }

    if (server_port <= 0 || server_port > 65535)
    {
        printf("Invalid PORT in '%s'.\n", CONFIG_FILE);
        return 0;
    }

    return 1;
}


int is_conversation_exit_command(const char *message)
{
    return strcasecmp(message, "eot\n") == 0 ||
           strcasecmp(message, "eol\n") == 0;
}


int is_ask_command(const char *command)
{
    return strcasecmp(command, "ask\n") == 0;
}


int is_help_command(const char *command)
{
    return strcasecmp(command, "help\n") == 0;
}


int is_status_command(const char *command)
{
    return strcasecmp(command, "status\n") == 0;
}


int is_shell_exit_command(const char *command)
{
    return strcasecmp(command, "quit\n") == 0 ||
           strcasecmp(command, "exit\n") == 0 ||
           strcasecmp(command, "bye\n") == 0;
}


void print_help(void)
{
    printf("\n");
    printf("Available commands:\n");
    printf("\n");
    printf("  ASK       Start a conversation with NeXTBrain\n");
    printf("  HELP      Display this help\n");
    printf("  STATUS    Display NeXTBrain server status\n");
    printf("  QUIT      Exit nbclient\n");
    printf("  EXIT      Exit nbclient\n");
    printf("  BYE       Exit nbclient\n");
    printf("\n");
    printf("While in ASK mode, use EOT or EOL to end the conversation.\n");
    printf("\n");
}


int main(void)
{
    int sock;
    int bytes_received;
    struct sockaddr_in server_address;

    char command[256];
    char message[4096];
    char buffer[4096];

    if (!load_configuration())
    {
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        printf("Could not create socket.\n");
        return 1;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    server_address.sin_addr.s_addr = inet_addr(server_host);

    if (connect(
            sock,
            (struct sockaddr *)&server_address,
            sizeof(server_address)
        ) < 0)
    {
        printf(
            "Could not connect to NeXTBrain at %s:%d.\n",
            server_host,
            server_port
        );

        return 1;
    }

    printf("Connected to NeXTBrain.\n\n");

    while (1)
    {
        printf("> ");
        fflush(stdout);

        if (fgets(command, sizeof(command), stdin) == NULL)
        {
            break;
        }

        if (is_ask_command(command))
        {
            while (1)
            {
                printf("ASK> ");
                fflush(stdout);

                if (fgets(message, sizeof(message), stdin) == NULL)
                {
                    return 0;
                }

                if (is_conversation_exit_command(message))
                {
                    break;
                }

                if (send(sock, message, strlen(message), 0) < 0)
                {
                    printf("Could not send message.\n");
                    return 1;
                }

                bytes_received = recv(
                    sock,
                    buffer,
                    sizeof(buffer) - 1,
                    0
                );

                if (bytes_received < 0)
                {
                    printf("Could not receive response.\n");
                    return 1;
                }

                if (bytes_received == 0)
                {
                    printf("NeXTBrain closed the connection.\n");
                    return 1;
                }

                buffer[bytes_received] = '\0';

                printf("\n%s\n", buffer);
            }
        }
        else if (is_help_command(command))
        {
            print_help();
        }
        else if (is_status_command(command))
        {
            printf("STATUS not implemented yet.\n");
        }
        else if (is_shell_exit_command(command))
        {
            break;
        }
        else
        {
            printf("Unknown command. Type HELP for available commands.\n");
        }
    }

    return 0;
}