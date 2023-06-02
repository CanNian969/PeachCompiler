#include"compiler.h"
#include"helpers/buffer.h"
#include"helpers/vector.h"
#include<string.h>

#define LEX_GETC_IF(buffer, c, exp)     \
    for(c = peekc(); exp; c = peekc())  \
    {                                   \
        buffer_write(buffer, c);        \
        nextc();                        \
    }                                   \

static struct lex_process* lexer;
static struct token tmp_token;
struct token* read_next_token();

static char peekc()
{
    return lexer->function->peek_char(lexer);
}

static char nextc()
{
    char c = lexer->function->next_char(lexer);
    lexer->pos.col += 1;
    if(c == '\n')
    {
        lexer->pos.col = 1;
        lexer->pos.line += 1;
    }
    return c;
}

static void pushc(char c)
{
    lexer->function->push_char(lexer, c);
}

static struct pos lex_file_position()
{
    return lexer->pos;
}

struct token* token_create(struct token* _token)
{
    memcpy(&tmp_token, _token, sizeof(struct token));
    tmp_token.pos = lex_file_position();
    return &tmp_token;
}

static struct token* lex_last_token()
{
    return vector_back_or_null(lexer->token_vec);
}

static struct token* handle_whitespace()
{
    struct token* last_token = lex_last_token();
    if(last_token)
        last_token->whitespace = true;

    nextc();
    return read_next_token();
}

const char* read_number_str()
{
    struct buffer* buffer = buffer_create();
    char c = peekc();
    LEX_GETC_IF(buffer, c, ('0' <= c && c <= '9'));
    
    buffer_write(buffer, 0x00);
    return buffer_ptr(buffer);
}

unsigned long long read_number()
{
    const char *num = read_number_str();
    return atoll(num);
}

struct token* token_make_number_for_value(unsigned long long number)
{
    return token_create(&(struct token){.type = TOKEN_TYPE_NUMBER, .llnum = number});
}

// token:: make函数 number类型
struct token* token_make_number()
{
    return token_make_number_for_value(read_number());
}

struct token* read_next_token()
{
    struct token* token = NULL;
    char c = peekc();
    switch(c)
    {
        NUMERIC_CASE:
            token = token_make_number();
            break;
        case ' ':
        case '\t':
            token = handle_whitespace();
            break;
        case EOF:
            // 读取结束
            break;
        default:
            compiler_error(lexer->compiler, "Unexpected token!");
    }
    
    return token;
}



int lex(struct lex_process* process)
{
    process->current_expression_count = 0;
    process->parentheses_buffer = NULL;
    process->pos.filename = process->compiler->ifile.abs_path;
    lexer = process;

    struct token* token = read_next_token();
    while(token)
    {
        vector_push(lexer->token_vec, token);
        token = read_next_token();
    }

    return LEXICAL_ANALYSIS_ALL_OK;
}