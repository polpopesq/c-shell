#define _POSIX_C_SOURCE 200809L // to show strtok_r definition
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum Command
{
  EXIT,
  ECHO,
  TYPE,
  UNKNOWN
};

enum Command get_command_type(const char *s)
{
  if (strcmp(s, "exit") == 0)
    return EXIT;
  if (strcmp(s, "echo") == 0)
    return ECHO;
  if (strcmp(s, "type") == 0)
    return TYPE;
  return UNKNOWN;
}

typedef struct
{
  char *cmd;
  char *args;
  enum Command type;
} ParsedCommand;

ParsedCommand parse_command(char *line)
{
  line[strcspn(line, "\n")] = '\0';

  ParsedCommand out = {0};
  char *space = strchr(line, " ");

  if (space)
  {
    *space = '\0';
    out.cmd = line;
    out.args = space + 1;
  }
  else
  {
    out.cmd = line;
    out.args = NULL;
  }

  out.type = get_command_type(out.cmd);
  return out;
}

void echo(const char *string)
{
  char *rest = string;
  char *token;
  while ((token = strtok_r(rest, " ", &rest)))
  {
    printf("%s ", token);
  }
  printf("\n");
}

void type(const char *string)
{
  printf(string);
  printf(get_command_type(string) == UNKNOWN ? ": not found\n" : " is a shell function\n");
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

    ParsedCommand command = parse_command(buffer);
    switch (command.type)
    {
    case EXIT:
      return 0;
    case ECHO:
      echo(command.args);
      break;
    case TYPE:
      type(command.args);
    default:
      printf("%s: command not found\n", command);
      break;
    }
  }
  return 0;
}
