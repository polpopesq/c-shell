#include "lexer.h"

#include <stdlib.h>
#include <string.h>

typedef enum { ST_NORMAL, ST_SQUOTE, ST_DQUOTE, ST_ESCAPE } ParseState;

static void emit_token(char*** tokens, int* ntokens, char* buffer, int* bi) {
    buffer[*bi] = '\0';

    (*tokens)[*ntokens] = strdup(buffer);
    (*ntokens)++;

    *bi = 0;
}

// tokens array must be freed by caller
// token strings are owned by caller or transferred
char** lex_tokens(char* line) {
    char** tokens = calloc(32, sizeof(char*));
    if (tokens == NULL) return NULL;

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
                    if (bi > 0) emit_token(&tokens, &ntokens, buffer, &bi);
                } else if (c == '|') {
                    if (bi > 0) emit_token(&tokens, &ntokens, buffer, &bi);
                    tokens[ntokens++] = strdup("|");
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