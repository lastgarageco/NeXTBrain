#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int is_exit_command(const char *message)
{
    return strcasecmp(message, "quit\n") == 0 ||
           strcasecmp(message, "eot\n") == 0 ||
           strcasecmp(message, "eol\n") == 0;
}

int main(void)
{
  int sock;
  int bytes_received;

  struct sockaddr_in server_address;

  char command[26];
  char message[4096]; 
  char buffer[4096];

  sock = socket(AF_INET, SOCK_STREAM,0);

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

  printf("Connected to NeXTBrain.\n");  

  while (1)
  {
    printf("> ");
    fgets(command, sizeof(command), stdin);

    if (strcmp(command, "ASK\n") == 0)
    {
      while(1)
      {
        printf("ASK> ");
        fgets(message, sizeof(message), stdin);

        if (is_exit_command(message))
        {
          break;
        }

        if (send(sock, message, strlen(message), 0) < 0)
        {
          printf("Could not send message.\n");
          return 1;
        }

        bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received < 0)
        {
          printf("Could not receive response.\n");
          return 1;
        }
  
        buffer[bytes_received] = '\0';
 
        printf(">>> \n%s",buffer);
      }
    }
  }

  return 0;
} 
