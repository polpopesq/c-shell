#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

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

typedef enum
{
    ST_NORMAL,
    ST_SQUOTE,
    ST_DQUOTE,
    ST_ESCAPE
} ParseState;

static void push_arg(ParsedCommand *out, char *buffer, int *bi)
{
    buffer[*bi] = '\0';
    out->argv[out->argc++] = strdup(buffer);
    *bi = 0;
}

ParsedCommand parse_command(char *line)
{
    ParsedCommand out = {0};
    out.cmd = strdup(line);
    char buffer[512];
    int bi = 0;

    ParseState st = ST_NORMAL;
    ParseState prev = ST_NORMAL;
    char *p = line;

    while (*p != '\0')
    {
        char c = *p++;
        switch (st)
        {
        case ST_NORMAL:
            if (c == ' ')
            {
                if (bi > 0)
                {
                    push_arg(&out, buffer, &bi);
                }
            }
            else if (c == '\'')
            {
                st = ST_SQUOTE;
            }
            else if (c == '"')
            {
                st = ST_DQUOTE;
            }
            else if (c == '\\')
            {
                prev = ST_NORMAL;
                st = ST_ESCAPE;
            }
            else
            {
                buffer[bi++] = c;
            }
            break;
        case ST_SQUOTE:
            if (c == '\'')
            {
                st = ST_NORMAL;
            }
            else
            {
                buffer[bi++] = c;
            }
            break;
        case ST_DQUOTE:
            if (c == '"')
            {
                st = ST_NORMAL;
            }
            else if (c == '\\')
            {
                char next = *p;
                if (next == '"' || next == '\\' || next == '`' || next == '$' || next == '*' || next == '?' || next == '\n')
                {
                    buffer[bi++] = next;
                    p++;
                }
                else
                {
                    prev = ST_DQUOTE;
                    st = ST_ESCAPE;
                }
            }
            else
            {
                buffer[bi++] = c;
            }
            break;
        case ST_ESCAPE:
            buffer[bi++] = c;
            st = prev;
            break;
        }
    }

    if (bi > 0)
    {
        push_arg(&out, buffer, &bi);
    }

    out.argv[out.argc] = NULL;

    if (out.argc > 0)
        out.type = get_command_type(out.argv[0]);
    else
        out.type = OTHER;

    return out;
}
int main()
{
    char command[256];
    fgets(command, 255, stdin);
    ParsedCommand pc = parse_command(command);
    for (int i = 0; i < pc.argc; ++i)
    {
        printf("arg %i: %s\n", i, pc.argv[i]);
    }
    return 0;
}