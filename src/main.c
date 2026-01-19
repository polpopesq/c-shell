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
  PWD,
  CD,
  OTHER
};

enum Command get_command_type(const char *s)
{
  if (strcmp(s, "exit") == 0)
    return EXIT;
  if (strcmp(s, "echo") == 0)
    return ECHO;
  if (strcmp(s, "type") == 0)
    return TYPE;
  if (strcmp(s, "pwd") == 0)
    return PWD;
  if (strcmp(s, "cd") == 0)
    return CD;
  return OTHER;
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

  int argc = 0;
  char *p = line;
  char buffer[256];
  int bi = 0;

  int in_quote = 0;

  while (*p && argc < 15)
  {
    // skip leading spaces if not building a token
    if (bi == 0)
    {
      while (*p == ' ')
        p++;
      if (*p == '\0')
        break;
    }

    if (*p == '\'') // if '
    {
      in_quote = !in_quote; // toggle quote state
      p++;
      continue;
    }

    if (!in_quote && *p == ' ') // if space outside quote <=> separate argument
    {
      buffer[bi] = '\0';
      out.argv[argc++] = strdup(buffer);
      bi = 0;

      while (*p == ' ')
        p++; // skip extra spaces
      continue;
    }

    buffer[bi++] = *p++;
  }

  // last argument
  if (bi > 0)
  {
    buffer[bi] = '\0';
    out.argv[argc++] = strdup(buffer);
  }

  out.argv[argc] = NULL;
  out.argc = argc;

  if (argc > 0)
  {
    out.cmd = out.argv[0];
    out.type = get_command_type(out.cmd);
  }

  return out;
}

char *path_lookup(const char *PATH, const char *command)
{
  if (!PATH || !command || !*command)
    return NULL;

  char *path_copy = strdup(PATH);
  if (!path_copy)
    return NULL;

  char *saveptr = NULL;
  char *dir = strtok_r(path_copy, PATH_LIST_SEPARATOR, &saveptr);

  while (dir)
  {
    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, command);

    if (access(fullpath, X_OK) == 0)
    {
      char *result = strdup(fullpath);
      free(path_copy);
      return result;
    }

    dir = strtok_r(NULL, PATH_LIST_SEPARATOR, &saveptr);
  }

  free(path_copy);
  return NULL;
}

void exec_type(const char *s, const char *PATH)
{
  if (get_command_type(s) == OTHER)
  {
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
    fprintf(stderr, "%s: command not found\n", argv[0]);
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

static void exec_pwd(void)
{
  char *buffer = getcwd(NULL, 0);
  if (buffer == NULL)
  {
    perror("getcwd");
    return;
  }

  printf("%s\n", buffer);
  free(buffer);
}

static void exec_cd(const char *target)
{
  if (strcmp(target, "~") == 0)
  {
    target = getenv("HOME");
  }

  if (chdir(target) != 0)
  {
    char msg[1024];
    snprintf(msg, sizeof(msg), "cd: %s", target);
    perror(msg);
  }
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
      exec_type(command.argv[1], PATH);
      break;
    case PWD:
      exec_pwd();
      break;
    case CD:
      if (command.argc < 2)
      {
        exec_cd("~");
      }
      else if (command.argc > 2)
      {
        printf("%s: too many arguments\n", command.argv[0]);
      }
      else
      {
        exec_cd(command.argv[1]);
      }
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
