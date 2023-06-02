#include"compiler.h"
#include"helpers/vector.h"

struct lex_process* lex_process_create(struct compile_process* compiler, struct lex_process_functions* function, void* private)
{
    struct lex_process* lexer = calloc(1, sizeof(struct lex_process));
    lexer->compiler = compiler;
    lexer->function = function;
    lexer->private = private;
    lexer->token_vec = vector_create(sizeof(struct token));
    lexer->pos.line = 1;
    lexer->pos.col = 1;
    return lexer;
}

void lex_process_free(struct lex_process* lexer)
{
    vector_free(lexer->token_vec);
    free(lexer);
}

void* lex_process_private(struct lex_process* lexer)
{
    return lexer->private;
}

struct vector* lex_process_tokens(struct lex_process* lexer)
{
    return lexer->token_vec;
}


