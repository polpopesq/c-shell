#define _POSIX_C_SOURCE 200809L // to show strtok_r definition
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum Command
{
  EXIT,
  ECHO,
  UNKNOWN
};

enum Command get_command_type(const char *s)
{
  if (strcmp(s, "exit") == 0)
  {
    return EXIT;
  }
  if (strcmp(s, "echo") == 0)
  {
    return ECHO;
  }
  return UNKNOWN;
}

void echo(const char *string)
{
  char *saveptr;
  char *token = strtok_r(string, " ", &saveptr);
  while ((token = strtok_r(NULL, " ", &saveptr)))
  {
    printf("%s ", token);
  }
  printf("\n");
}

int main(int argc, char *argv[])
{
  // Flush after every printf
  setbuf(stdout, NULL);
  char buffer[1024];

  while (1)
  {
    printf("$ ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = '\0';

    char command[64];
    if (sscanf(buffer, "%s", command) != EOF)
    {
      enum Command command_type = get_command_type(command);
      switch (command_type)
      {
      case EXIT:
        return 0;
      case ECHO:
        echo(buffer);
        break;
      default:
        printf("%s: command not found\n", command);
        break;
      }
    }
  }
  return 0;
}
