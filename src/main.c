#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

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
  char *argv[16]; // tokenized command + args
  int argc;
  enum Command type;
} ParsedCommand;

ParsedCommand parse_command(char *line)
{
  line[strcspn(line, "\n")] = '\0';

  ParsedCommand out = {0};

  int i = 0;
  char *token = strtok(line, " ");

  while (token && i < 15)
  {
    out.argv[i++] = token;
    token = strtok(NULL, " ");
  }
  out.argv[i] = NULL;
  out.argc = i;

  if (i > 0)
  {
    out.cmd = out.argv[0];
    out.type = get_command_type(out.cmd);
  }

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

void type(const char *s, const char *PATH)
{
  if (!s)
    return;

  if (get_command_type(s) == UNKNOWN)
  {
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

static int exec_prog(char *const argv[])
{
  pid_t pid = fork();

  if (pid == -1)
  {
    perror("fork failed");
    return -1;
  }

  if (pid == 0)
  {
    execvp(argv[0], argv);
    perror("execvp failed");
    _exit(127);
  }

  // parent: wait for child to finish
  int status;
  if (waitpid(pid, &status, 0) == -1)
  {
    perror("waitpid failed");
    return -1;
  }

  if (WIFEXITED(status))
  {
    return WEXITSTATUS(status);
  }

  return -1;
}

int main(int argc, char *argv[])
{
  // Flush after every printf
  setbuf(stdout, NULL);
  char buffer[1024];
  const char *PATH = getenv("PATH");

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
      for (int i = 1; i < command.argc; i++)
      {
        printf("%s ", command.argv[i]);
      }
      printf("\n");
      break;
    case TYPE:
      type(command.argv[1], PATH);
      break;
    default:
      if (path_lookup(PATH, command.cmd) == NULL)
      {
        printf("%s: command not found\n", command.cmd);
      }
      else
      {
        int rc = exec_prog(command.argv);
        if (rc != 0)
        {
          perror("child program failed to run");
        }
      }
      break;
    }
  }
  return 0;
}
