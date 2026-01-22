#include "parser.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse/lexer.h"

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
        fprintf(stderr, "syntax error: unknown redirection '%s'\n", token);
        return out;
    }

    out.filename = strdup(next_token);
    *ok = true;
    return out;
}

// parsed command should be freed by caller using free_parsed_command
ParsedCommand parse_command(char* line) {
    ParsedCommand out = {0};
    out.cmd = strdup(line);

    char** tokens = lex_tokens(line);
    if (tokens == NULL) {
        if (errno == ENOMEM) {
            fprintf(stderr, "allocation error: Not enough space or memory \n");
        } else {
            fprintf(stderr, "allocation error: Unknown error \n");
        }
    }

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
    free(tokens);  // freed tokens

    return out;
}