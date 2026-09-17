#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int is_conversation_exit_command(const char *message)
{
    return strcasecmp(message, "eot\n") == 0 ||
           strcasecmp(message, "eol\n") == 0;
}

int is_ask_command(const char *command)
{
    return strcasecmp(command, "ask\n") == 0;
}

int is_shell_exit_command(const char *command)
{
    return strcasecmp(command, "quit\n") == 0 ||
           strcasecmp(command, "exit\n") == 0 ||
           strcasecmp(command, "bye\n") == 0;
}

int is_help_command(const char *command)
{
    return strcasecmp(command, "help\n") == 0;
}

void print_help(void)
{
    printf("\n");
    printf("Available commands:\n");
    printf("\n");
    printf("  ASK    Start a conversation with NeXTBrain\n");
    printf("  HELP   Display this help\n");
    printf("  QUIT   Exit nbclient\n");
    printf("  EXIT   Exit nbclient\n");
    printf("  BYE    Exit nbclient\n");
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

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        printf("Could not create socket.\n");
        return 1;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(5555);
    server_address.sin_addr.s_addr = inet_addr("192.168.1.106");

    if (connect(
            sock,
            (struct sockaddr *)&server_address,
            sizeof(server_address)
        ) < 0)
    {
        printf("Could not connect to NeXTBrain.\n");
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
