#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>

#define _POSIX_C_SOURCE 200809L // to show strtok_r definition
#ifdef _WIN32
#define PATH_LIST_SEPARATOR ";"
#else
#define PATH_LIST_SEPARATOR ":"
#endif

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
  char *space = strchr(line, ' ');

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

bool check_dir_command(const char *dir_token, const char *command)
{
  DIR *dp = opendir(dir_token);
  if (dp == NULL)
    return false;

  struct dirent *entry;
  char path[1024];

  while ((entry = readdir(dp)) != NULL)
  {
    if (strcmp(entry->d_name, command) != 0)
      continue;

    snprintf(path, sizeof(path), "%s/%s", dir_token, entry->d_name);

    if (access(path, X_OK) == 0)
    {
      closedir(dp);
      return true;
    }
  }

  closedir(dp);
  return false;
}

char *path_lookup(const char *PATH, const char *command)
{
  if (!PATH || !command)
    return NULL;

  const char *current = PATH;

  while (*current)
  {
    current += strspn(current, PATH_LIST_SEPARATOR);
    if (!*current)
      break;

    int len = strcspn(current, PATH_LIST_SEPARATOR);

    char *dir = strndup(current, len);
    if (!dir)
      return NULL;

    char fullpath[512];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, command);

    if (access(fullpath, X_OK) == 0)
    {
      free(dir);
      return strdup(fullpath);
    }

    free(dir);
    current += len;
  }

  return NULL;
}

void type(const char *s)
{
  if (!s)
    return;

  if (get_command_type(s) == UNKNOWN)
  {
    const char *PATH = getenv("PATH");
    if (!PATH)
    {
      printf("%s: not found\n", s);
      return;
    }

    char *result = path_lookup(PATH, s);
    if (!result)
    {
      printf("%s: not found\n", s);
    }
    else
    {
      printf("%s is %s\n", s, result);
      free(result);
    }
  }
  else
  {
    printf("%s is a shell builtin\n", s);
  }
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
      printf("%s\n", command.args);
      break;
    case TYPE:
      type(command.args);
      break;
    default:
      printf("%s: command not found\n", command.cmd);
      break;
    }
  }
  return 0;
}
