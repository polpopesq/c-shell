#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define _POSIX_C_SOURCE 200809L  // to show strtok_r definition
#ifdef _WIN32
#define PATH_LIST_SEPARATOR ";"
#else
#define PATH_LIST_SEPARATOR ":"
#endif

#define MAX_REDIR 8
#define MAX_ARGV 16

typedef struct {
    int target_fd;
    enum { TRUNC, APPEND } mode;
    char* filename;
} Redirection;

typedef struct {
    char* cmd;
    char* argv[MAX_ARGV];  // tokenized command + args
    int argc;
    Redirection redirections[MAX_REDIR];
    int redirc;
} ParsedCommand;

typedef int (*builtin_func)(const ParsedCommand* cmd);
typedef struct {
    const char* name;       // command string, e.g., "cd"
    builtin_func function;  // function pointer
} builtin_entry;

int exec_cd(const ParsedCommand* cmd);
int exec_exit(const ParsedCommand* cmd);
int exec_pwd(const ParsedCommand* cmd);
int exec_type(const ParsedCommand* cmd);
int exec_echo(const ParsedCommand* cmd);

builtin_entry builtins[] = {
    {"cd", exec_cd},     {"pwd", exec_pwd},   {"echo", exec_echo},
    {"exit", exec_exit}, {"type", exec_type},
};

#define NUM_BUILTINS (sizeof(builtins) / sizeof(builtins[0]))

builtin_func find_builtin(const char* name) {
    for (size_t i = 0; i < NUM_BUILTINS; i++) {
        if (strcmp(name, builtins[i].name) == 0) {
            return builtins[i].function;
        }
    }
    return NULL;  // not found → external command
}

typedef enum { ST_NORMAL, ST_SQUOTE, ST_DQUOTE, ST_ESCAPE } ParseState;

void print_parsed(const ParsedCommand* cmd) {
    if (!cmd) {
        printf("ParsedCommand: (null)\n");
        return;
    }

    printf("============= ParsedCommand =============\n");

    printf("raw cmd : \"%s\"\n", cmd->cmd ? cmd->cmd : "(null)");

    printf("argc    : %d\n", cmd->argc);

    printf("argv    :\n");
    for (int i = 0; i < cmd->argc; i++) {
        printf("  argv[%d] = \"%s\"\n", i,
               cmd->argv[i] ? cmd->argv[i] : "(null)");
    }

    printf("redirections (%d):\n", cmd->redirc);
    for (int i = 0; i < cmd->redirc; i++) {
        const Redirection* r = &cmd->redirections[i];

        printf("  [%d] fd=%d, mode=%s, file=\"%s\"\n", i, r->target_fd,
               (r->mode == TRUNC) ? "TRUNC" : "APPEND",
               r->filename ? r->filename : "(null)");
    }

    printf("=========================================\n");
}

void free_redirection(Redirection* r) { free(r->filename); }

void free_parsed_command(ParsedCommand* pc) {
    free(pc->cmd);

    for (int i = 0; i < pc->argc; i++) {
        free(pc->argv[i]);
    }

    for (int i = 0; i < pc->redirc; i++) {
        free_redirection(&(pc->redirections[i]));
    }
}

static void emit_token(char*** tokens, int* ntokens, char* buffer, int* bi) {
    buffer[*bi] = '\0';

    (*tokens)[*ntokens] = strdup(buffer);
    (*ntokens)++;

    *bi = 0;
}

// tokens array must be freed by caller
// token strings are owned by caller or transferred
static char** lex_tokens(char* line) {
    char** tokens = calloc(32, sizeof(char*));
    int ntokens = 0;

    char buffer[512];
    int bi = 0;

    ParseState st = ST_NORMAL;
    ParseState prev = ST_NORMAL;
    char* p = line;

    while (*p != '\0') {
        char c = *p++;

        switch (st) {
            case ST_NORMAL:
                if (c == ' ') {
                    if (bi > 0) {
                        emit_token(&tokens, &ntokens, buffer, &bi);
                    }
                } else if (c == '\'') {
                    st = ST_SQUOTE;
                } else if (c == '"') {
                    st = ST_DQUOTE;
                } else if (c == '\\') {
                    prev = ST_NORMAL;
                    st = ST_ESCAPE;
                } else {
                    buffer[bi++] = c;
                }
                break;

            case ST_SQUOTE:
                if (c == '\'') {
                    st = ST_NORMAL;
                } else {
                    buffer[bi++] = c;
                }
                break;

            case ST_DQUOTE:
                if (c == '"') {
                    st = ST_NORMAL;
                } else if (c == '\\') {
                    char next = *p;
                    if (next == '"' || next == '\\' || next == '`' ||
                        next == '$' || next == '*' || next == '?' ||
                        next == '\n') {
                        buffer[bi++] = next;
                        p++;
                    } else {
                        buffer[bi++] = '\\';
                    }
                } else {
                    buffer[bi++] = c;
                }
                break;

            case ST_ESCAPE:
                buffer[bi++] = c;
                st = prev;
                break;
        }
    }

    if (bi > 0) {
        emit_token(&tokens, &ntokens, buffer, &bi);
    }

    tokens[ntokens] = NULL;
    return tokens;
}

bool is_redirection(const char* token) {
    if (token == NULL) {
        return false;
    }

    return strcmp(token, ">") == 0 || strcmp(token, "1>") == 0 ||
           strcmp(token, ">>") == 0 || strcmp(token, "1>>") == 0 ||
           strcmp(token, "2>") == 0 || strcmp(token, "2>>") == 0;
}

Redirection parse_redirection(const char* token, char* next_token, bool* ok) {
    Redirection out = {0};
    *ok = false;

    if (token == NULL || next_token == NULL) {
        fprintf(stderr, "syntax error: redirection without target\n");
        return out;
    }

    if (strcmp(token, ">") == 0 || strcmp(token, "1>") == 0) {
        out.target_fd = 1;
        out.mode = TRUNC;
    } else if (strcmp(token, ">>") == 0 || strcmp(token, "1>>") == 0) {
        out.target_fd = 1;
        out.mode = APPEND;
    } else if (strcmp(token, "2>") == 0) {
        out.target_fd = 2;
        out.mode = TRUNC;
    } else if (strcmp(token, "2>>") == 0) {
        out.target_fd = 2;
        out.mode = APPEND;
    } else {
        fprintf(stderr, "internal error: unknown redirection '%s'\n", token);
        return out;
    }

    out.filename = strdup(next_token);
    *ok = true;
    return out;
}

ParsedCommand parse_command(char* line) {
    ParsedCommand out = {0};
    out.cmd = strdup(line);

    char** tokens = lex_tokens(line);

    int i = 0;
    while (tokens[i] != NULL) {
        if (is_redirection(tokens[i])) {
            bool ok;
            Redirection r = parse_redirection(tokens[i], tokens[i + 1], &ok);

            if (!ok) {
                break;
            }

            out.redirections[out.redirc++] = r;
            i += 2;  // skip operator + filename
        } else {
            out.argv[out.argc++] = tokens[i];
            i += 1;
        }
    }

    out.argv[out.argc] = NULL;
    free(tokens);

    return out;
}

char* path_lookup(const char* command) {
    if (!command || !*command) return NULL;

    char* PATH = getenv("PATH");
    if (PATH == NULL) return NULL;
    char* path = strdup(PATH);

    char* saveptr = NULL;
    char* dir = strtok_r(path, PATH_LIST_SEPARATOR, &saveptr);

    while (dir) {
        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, command);

        if (access(fullpath, X_OK) == 0) {
            char* result = strdup(fullpath);
            free(path);
            return result;
        }

        dir = strtok_r(NULL, PATH_LIST_SEPARATOR, &saveptr);
    }

    free(path);
    return NULL;
}

void restore_fds(int saved_fds[3]) {
    dup2(saved_fds[0], STDIN_FILENO);
    dup2(saved_fds[1], STDOUT_FILENO);
    dup2(saved_fds[2], STDERR_FILENO);

    close(saved_fds[0]);
    close(saved_fds[1]);
    close(saved_fds[2]);
}

int apply_redirections(const ParsedCommand* cmd, int saved_fds[3]) {
    if (saved_fds != NULL) {
        saved_fds[0] = dup(STDIN_FILENO);
        saved_fds[1] = dup(STDOUT_FILENO);
        saved_fds[2] = dup(STDERR_FILENO);

        if (saved_fds[0] < 0 || saved_fds[1] < 0 || saved_fds[2] < 0) {
            perror("dup");
            return -1;
        }
    }

    for (int i = 0; i < cmd->redirc; ++i) {
        const Redirection* r = &cmd->redirections[i];
        int flags = O_WRONLY | O_CREAT;
        if (r->mode == TRUNC) {
            flags |= O_TRUNC;
        } else {
            flags |= O_APPEND;
        }

        int fd = open(r->filename, flags, 0644);
        if (fd < 0) {
            perror(r->filename);
            if (saved_fds != NULL) restore_fds(saved_fds);
            return -1;
        }
        if (dup2(fd, r->target_fd) < 0) {
            perror("dup2");
            close(fd);
            return -1;
        }

        close(fd);
    }

    return 0;
}

int exec_type(const ParsedCommand* cmd) {
    if (cmd->argc < 2 || cmd->argv[1] == NULL) {
        return 2;
    }
    if (find_builtin(cmd->argv[1]) == NULL) {
        char* result = path_lookup(cmd->argv[1]);
        if (!result) {
            printf("%s: not found\n", cmd->argv[1]);
        } else {
            printf("%s is %s\n", cmd->argv[1], result);
            free(result);
        }
    } else {
        printf("%s is a shell builtin\n", cmd->argv[1]);
    }
    return 0;
}

int exec_pwd(const ParsedCommand* cmd) {
    char* buffer = getcwd(NULL, 0);
    if (buffer == NULL) {
        perror("getcwd");
        return -1;
    }

    printf("%s\n", buffer);
    free(buffer);
    return 0;
}

int exec_cd(const ParsedCommand* cmd) {
    const char* target = cmd->argv[1];
    if (target == NULL || (strcmp(target, "~") == 0)) {
        target = getenv("HOME");
    }

    if (chdir(target) != 0) {
        char msg[1024];
        snprintf(msg, sizeof(msg), "cd: %s", target);
        perror(msg);
        return -1;
    }
    return 0;
}

int exec_exit(const ParsedCommand* cmd) {
    int code = 0;  // default exit code

    // If user provides an argument: exit <code>
    if (cmd->argc > 1) {
        char* endptr = NULL;
        code = strtol(cmd->argv[1], &endptr, 10);
        if (*endptr != '\0') {
            fprintf(stderr, "exit: numeric argument required\n");
            code = 1;
        }
    }

    // Optional: cleanup before exiting
    // free_parsed_command(cmd);  // if you want to free immediately
    // free any other global resources if needed

    exit(code);  // terminate shell
}

int exec_echo(const ParsedCommand* command) {
    for (int i = 1; i < command->argc; i++) {
        printf("%s ", command->argv[i]);
    }
    printf("\n");
    return 0;
}

int exec_builtin(builtin_func bf, const ParsedCommand* command) {
    int saved_fds[3];
    if (apply_redirections(command, saved_fds) != 0) {
        perror("apply_redirections");
        return -1;
    } else {
        int result = bf(command);
        restore_fds(saved_fds);
        return result;
    }
}

int exec_external(const ParsedCommand* command) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }
    // in the child process(0), no need to save and restore fds
    if (pid == 0) {
        if (apply_redirections(command, NULL) != 0) {
            _exit(1);
        }
        execvp(command->argv[0], command->argv);
        // only gets past this point if it fails.
        fprintf(stderr, "%s: command not found\n", command->argv[0]);
        _exit(127);
    }
    // parent
    int child_status;
    pid_t child = waitpid(pid, &child_status, 0);
    if (WIFEXITED(child_status)) {
        int code = WEXITSTATUS(child_status);
        return code;  // return child's exit code to caller
    } else if (WIFSIGNALED(child_status)) {
        int sig = WTERMSIG(child_status);
        fprintf(stderr, "Child killed by signal %d\n", sig);
        return 128 + sig;  // common shell convention
    }

    return -1;  // unknown termination
}

int execute_command(const ParsedCommand* cmd) {
    builtin_func bf = find_builtin(cmd->argv[0]);
    if (bf) {
        return exec_builtin(bf, cmd);
    } else {
        return exec_external(cmd);
    }
}

int main(int argc, char* argv[]) {
    setbuf(stdout, NULL);

    char buffer[1024];
    int last_status = 0;

    while (1) {
        printf("$ ");
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            putchar('\n');  // handle Ctrl-D / 0EOF
            break;
        }

        buffer[strcspn(buffer, "\n")] = '\0';
        if (buffer[0] == '\0') continue;

        ParsedCommand command = parse_command(buffer);
        int exit_code = execute_command(&command);
        // print_parsed(&command); //debug purposes

        // printf("\nEXIT CODE: %d\n", exit_code); //debug purposes

        free_parsed_command(&command);

        // for exit builtins, execute_command calls exit() internally
        // so we only reach here for normal commands
        (void)exit_code;  // marks variable as intentionally unused
    }

    return 0;
}