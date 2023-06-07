#include "compiler.h"

struct lex_process_functions compiler_lex_functions = {
    .next_char = compile_process_next_char,
    .peek_char = compile_process_peek_char,
    .push_char = compile_process_push_char
};


int compile_file(const char* filename, const char* out_filename, int flags)
{
    struct compile_process *cprocess = compile_process_create(filename, out_filename, flags);
    if(!cprocess)
        return COMPILER_FAILED_WITH_ERROR;

    // Preform lexical analysis
    struct lex_process *lexer = lex_process_create(cprocess, &compiler_lex_functions, NULL);
    if(!lexer)
        return COMPILER_FAILED_WITH_ERROR;

    if(lex(lexer) != LEXICAL_ANALYSIS_ALL_OK)
        return COMPILER_FAILED_WITH_ERROR;
    cprocess->token_vec = lexer->token_vec;
    // Preform parsing

    // Preform code generator

    return COMPILER_FILE_COMPILED_OK;
};