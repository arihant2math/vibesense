#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <strtod.h>
#include <string.h>
#include <math.h>

typedef enum {
    TOKEN_NUMBER,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_END,
    TOKEN_INVALID
} TokenType;

typedef struct {
    TokenType type;
    double value;
    size_t position;
} Token;

typedef struct {
    Token *items;
    size_t count;
    size_t capacity;
} TokenList;

typedef enum {
    EVAL_OK = 0,
    EVAL_OUT_OF_MEMORY,
    EVAL_INVALID_TOKEN,
    EVAL_UNEXPECTED_TOKEN,
    EVAL_UNEXPECTED_END,
    EVAL_DIVISION_BY_ZERO,
    EVAL_INVALID_EXPRESSION
} EvalError;

typedef struct {
    EvalError error;
    size_t position;
    double value;
} EvalResult;

static int token_list_push(TokenList *list, Token token) {
    if (list->count == list->capacity) {
        size_t capacity = list->capacity ? list->capacity * 2 : 16;
        Token *items = realloc(list->items, capacity * sizeof(*items));
        if (!items) {
            return 0;
        }
        list->items = items;
        list->capacity = capacity;
    }

    list->items[list->count++] = token;
    return 1;
}

void free_tokens(TokenList *tokens) {
    if (!tokens) {
        return;
    }

    free(tokens->items);
    tokens->items = NULL;
    tokens->count = 0;
    tokens->capacity = 0;
}

EvalError tokenize(const char *source, TokenList *out) {
    size_t i = 0;

    if (!source || !out) {
        return EVAL_INVALID_EXPRESSION;
    }

    out->items = NULL;
    out->count = 0;
    out->capacity = 0;

    while (source[i]) {
        Token token = {0};
        char *end;
        double value;

        if (isspace((unsigned char)source[i])) {
            i++;
            continue;
        }

        token.position = i;

        if (isdigit((unsigned char)source[i]) || source[i] == '.') {
            errno = 0;
            value = strtod(source + i, &end);

            if (end == source + i || errno == ERANGE) {
                free_tokens(out);
                return EVAL_INVALID_TOKEN;
            }

            token.type = TOKEN_NUMBER;
            token.value = value;
            i = (size_t)(end - source);

            if (!token_list_push(out, token)) {
                free_tokens(out);
                return EVAL_OUT_OF_MEMORY;
            }

            continue;
        }

        switch (source[i]) {
            case '+': token.type = TOKEN_PLUS; break;
            case '-': token.type = TOKEN_MINUS; break;
            case '*': token.type = TOKEN_STAR; break;
            case '/': token.type = TOKEN_SLASH; break;
            case '(': token.type = TOKEN_LPAREN; break;
            case ')': token.type = TOKEN_RPAREN; break;
            default:
                free_tokens(out);
                return EVAL_INVALID_TOKEN;
        }

        i++;

        if (!token_list_push(out, token)) {
            free_tokens(out);
            return EVAL_OUT_OF_MEMORY;
        }
    }

    if (!token_list_push(out, (Token){ TOKEN_END, 0.0, i })) {
        free_tokens(out);
        return EVAL_OUT_OF_MEMORY;
    }

    return EVAL_OK;
}

typedef struct {
    const TokenList *tokens;
    size_t index;
    EvalError error;
    size_t error_position;
} Parser;

static const Token *current_token(const Parser *parser) {
    return &parser->tokens->items[parser->index];
}

static double parse_expression(Parser *parser);

static double parse_primary(Parser *parser) {
    const Token *token = current_token(parser);

    if (token->type == TOKEN_NUMBER) {
        parser->index++;
        return token->value;
    }

    if (token->type == TOKEN_LPAREN) {
        double value;

        parser->index++;
        value = parse_expression(parser);

        if (parser->error != EVAL_OK) {
            return 0.0;
        }

        if (current_token(parser)->type != TOKEN_RPAREN) {
            parser->error = current_token(parser)->type == TOKEN_END
                ? EVAL_UNEXPECTED_END
                : EVAL_UNEXPECTED_TOKEN;
            parser->error_position = current_token(parser)->position;
            return 0.0;
        }

        parser->index++;
        return value;
    }

    parser->error = token->type == TOKEN_END
        ? EVAL_UNEXPECTED_END
        : EVAL_UNEXPECTED_TOKEN;
    parser->error_position = token->position;
    return 0.0;
}

static double parse_unary(Parser *parser) {
    const Token *token = current_token(parser);

    if (token->type == TOKEN_PLUS) {
        parser->index++;
        return parse_unary(parser);
    }

    if (token->type == TOKEN_MINUS) {
        parser->index++;
        return -parse_unary(parser);
    }

    return parse_primary(parser);
}

static double parse_multiplication(Parser *parser) {
    double value = parse_unary(parser);

    while (parser->error == EVAL_OK) {
        const Token *token = current_token(parser);

        if (token->type != TOKEN_STAR && token->type != TOKEN_SLASH) {
            break;
        }

        parser->index++;

        {
            double rhs = parse_unary(parser);

            if (parser->error != EVAL_OK) {
                return 0.0;
            }

            if (token->type == TOKEN_SLASH) {
                if (rhs == 0.0) {
                    parser->error = EVAL_DIVISION_BY_ZERO;
                    parser->error_position = token->position;
                    return 0.0;
                }
                value /= rhs;
            } else {
                value *= rhs;
            }
        }
    }

    return value;
}

static double parse_expression(Parser *parser) {
    double value = parse_multiplication(parser);

    while (parser->error == EVAL_OK) {
        const Token *token = current_token(parser);

        if (token->type != TOKEN_PLUS && token->type != TOKEN_MINUS) {
            break;
        }

        parser->index++;

        {
            double rhs = parse_multiplication(parser);

            if (parser->error != EVAL_OK) {
                return 0.0;
            }

            if (token->type == TOKEN_PLUS) {
                value += rhs;
            } else {
                value -= rhs;
            }
        }
    }

    return value;
}

EvalResult evaluate_tokens(const TokenList *tokens) {
    Parser parser;
    double value;

    if (!tokens || !tokens->items || tokens->count == 0) {
        return (EvalResult){ EVAL_INVALID_EXPRESSION, 0, 0.0 };
    }

    parser.tokens = tokens;
    parser.index = 0;
    parser.error = EVAL_OK;
    parser.error_position = 0;

    value = parse_expression(&parser);

    if (parser.error != EVAL_OK) {
        return (EvalResult){
            parser.error,
            parser.error_position,
            0.0
        };
    }

    if (current_token(&parser)->type != TOKEN_END) {
        return (EvalResult){
            EVAL_UNEXPECTED_TOKEN,
            current_token(&parser)->position,
            0.0
        };
    }

    return (EvalResult){ EVAL_OK, 0, value };
}

EvalResult evaluate_expression(const char *source) {
    TokenList tokens = {0};
    EvalError error;
    EvalResult result;

    error = tokenize(source, &tokens);
    if (error != EVAL_OK) {
        return (EvalResult){ error, 0, 0.0 };
    }

    result = evaluate_tokens(&tokens);
    free_tokens(&tokens);
    return result;
}
