#include "parser.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse/lexer.h"

void free_redirection(Redirection* r) { free(r->filename); }

void free_parsed_command(Command* pc) {
    for (int i = 0; i < pc->argc; i++) {
        free(pc->argv[i]);
    }

    for (int i = 0; i < pc->redirc; i++) {
        free_redirection(&(pc->redirections[i]));
    }
}

void free_pipeline(Pipeline* pl) {
    for (int i = 0; i < pl->count; ++i) {
        free_parsed_command(&(pl->cmds[i]));
    }
    free(pl->cmds);
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

    if (next_token == NULL) {
        fprintf(stderr, "syntax error: redirection without target\n");
        return out;
    }

    if (strcmp(token, "<") == 0) {
        out.target_fd = 0;
        out.mode = READ;
    } else if (strcmp(token, ">") == 0 || strcmp(token, "1>") == 0) {
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
    }

    out.filename = strdup(next_token);
    *ok = true;
    return out;
}

Command parse_command_tokens(char** tokens, int start, int end) {
    Command out = {0};

    int i = start;
    while (i < end && tokens[i] != NULL) {
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

    return out;
}

// parsed pipeline should be freed by caller using free_pipeline
Pipeline parse_pipeline(char* line) {
    Pipeline out = {0};
    char** tokens = lex_tokens(line);
    if (!tokens) return out;

    int start = 0;

    for (int i = 0;; i++) {
        if (tokens[i] == NULL || strcmp(tokens[i], "|") == 0) {
            // syntax error: empty command
            if (i == start) {
                fprintf(stderr, "syntax error near '|'\n");
                free_pipeline(&out);
                free(tokens);
                return (Pipeline){0};
            }

            // grow command array
            Command* tmp = realloc(out.cmds, sizeof(Command) * (out.count + 1));
            if (!tmp) {
                perror("realloc");
                free_pipeline(&out);
                free(tokens);
                return (Pipeline){0};
            }
            out.cmds = tmp;

            // parse one command segment
            out.cmds[out.count++] = parse_command_tokens(tokens, start, i);

            // end of input → stop
            if (tokens[i] == NULL) break;

            // skip '|'
            start = i + 1;
        }
    }

    free(tokens);
    return out;
}
