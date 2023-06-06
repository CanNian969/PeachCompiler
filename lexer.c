#include "compiler.h"
#include "helpers/buffer.h"
#include "helpers/vector.h"
#include <string.h>
#include <assert.h>
#include <ctype.h>

#define LEX_GETC_IF(buffer, c, exp)     \
    for (c = peekc(); exp; c = peekc()) \
    {                                   \
        buffer_write(buffer, c);        \
        nextc();                        \
    }

static struct lex_process *lexer;
static struct token tmp_token;
struct token *read_next_token();

static char peekc()
{
    return lexer->function->peek_char(lexer);
}

static char nextc()
{
    char c = lexer->function->next_char(lexer);
    lexer->pos.col += 1;
    if (c == '\n')
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

struct token *token_create(struct token *_token)
{
    memcpy(&tmp_token, _token, sizeof(struct token));
    tmp_token.pos = lex_file_position();
    return &tmp_token;
}

static struct token *lex_last_token()
{
    return vector_back_or_null(lexer->token_vec);
}

static struct token *handle_whitespace()
{
    struct token *last_token = lex_last_token();
    if (last_token)
        last_token->whitespace = true;

    nextc();
    return read_next_token();
}

const char *read_number_str()
{
    struct buffer *buffer = buffer_create();
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

struct token *token_make_number_for_value(unsigned long long number)
{
    return token_create(&(struct token){.type = TOKEN_TYPE_NUMBER, .llnum = number});
}

// token:: make函数 number类型
struct token *token_make_number()
{
    return token_make_number_for_value(read_number());
}

struct token *token_make_string(char start_delmt, char end_delmt)
{
    struct buffer *buff = buffer_create();
    assert(nextc() == start_delmt);
    char c = nextc();
    for (; c != end_delmt && c != EOF; c = nextc())
    {
        if (c == '\\')
        {
            // 跳过 反斜杠'\'
            continue;
        }
        buffer_write(buff, c);
    }
    buffer_write(buff, 0x00);
    return token_create(&(struct token){.type = TOKEN_TYPE_STRING, .svar = buffer_ptr(buff)});
}

static bool op_treated_as_one(char op)
{
    return op == '(' || op == '[' || op == ',' || op == '*' || op == '.' || op == '?';
}

static bool is_single_operator(char op)
{
    return op == '+' || op == '-' || op == '/' || op == '*' || op == '=' || op == '<' ||
           op == '>' || op == '|' || op == '&' || op == '^' || op == '%' || op == '!' ||
           op == '(' || op == '[' || op == ',' || op == '.' || op == '~' || op == '?';
}

static bool op_valid(const char *op)
{
    return S_EQ(op, "+") || S_EQ(op, "-") || S_EQ(op, "*") || S_EQ(op, "/") ||
           S_EQ(op, "!") || S_EQ(op, "^") || S_EQ(op, "+=") || S_EQ(op, "-=") ||
           S_EQ(op, "*=") || S_EQ(op, ">>=") || S_EQ(op, "<<=") || S_EQ(op, "/=") ||
           S_EQ(op, ">>") || S_EQ(op, "<<") || S_EQ(op, ">=") || S_EQ(op, "<=") ||
           S_EQ(op, "<") || S_EQ(op, "||") || S_EQ(op, "&&") || S_EQ(op, "|") ||
           S_EQ(op, "&") || S_EQ(op, "++") || S_EQ(op, "--") || S_EQ(op, "=") ||
           S_EQ(op, "*=") || S_EQ(op, "^=") || S_EQ(op, "==") || S_EQ(op, "!=") ||
           S_EQ(op, "->") || S_EQ(op, "**") || S_EQ(op, "(") || S_EQ(op, "[") ||
           S_EQ(op, ",") || S_EQ(op, ".") || S_EQ(op, "...") || S_EQ(op, "~") ||
           S_EQ(op, "?") || S_EQ(op, "%") || S_EQ(op, ">");
}

// 当遇到+*这种操作符合法但连在一起不合法时，需要只留下+，将后面的flush回去留给下一次token
void read_op_flush_back_keep_first(struct buffer *buff)
{
    const char *data = buffer_ptr(buff);
    int len = buff->len;
    for (int i = len - 1; i >= 1; i--)
    {
        if (data[i] == 0x00)
            continue;
        pushc(data[i]);
    }
}

const char *read_op()
{
    bool single_oprator = true;
    struct buffer *buff = buffer_create();
    char op = nextc();
    buffer_write(buff, op);

    if (!op_treated_as_one(op))
    {
        op = peekc();
        if (is_single_operator(op))
        {
            buffer_write(buff, op);
            nextc();
            single_oprator = false;
        }
    }
    buffer_write(buff, 0x00);
    char *ptr = buffer_ptr(buff);

    if (!single_oprator)
    {
        if (!op_valid(ptr))
        {
            read_op_flush_back_keep_first(buff);
            ptr[1] = 0x00;
        }
    }
    else if (!op_valid(ptr))
    {
        compiler_error(lexer->compiler, "The operator %s is not valid", ptr);
    }
    return ptr;
}

// ( ( exp ) ) 处理多括号多表达式的情况
static void lex_new_expression()
{
    lexer->current_expression_count++;
    if (lexer->current_expression_count == 1)
    {
        lexer->parentheses_buffer = buffer_create();
    }
}

static void lex_finish_expression()
{
    lexer->current_expression_count--;
    if (lexer->current_expression_count < 0)
    {
        compiler_error(lexer->compiler, "You closed an expression that you never opened\n");
    }
}

// 判断我们是否在expression中
bool lex_is_in_expression()
{
    return lexer->current_expression_count > 0;
}

bool is_keyword(const char *str)
{
    return S_EQ(str, "unsigned") || S_EQ(str, "signed") || S_EQ(str, "char") || S_EQ(str, "short") ||
           S_EQ(str, "int") || S_EQ(str, "float") || S_EQ(str, "double") || S_EQ(str, "long") ||
           S_EQ(str, "void") || S_EQ(str, "struct") || S_EQ(str, "union") || S_EQ(str, "static") ||
           S_EQ(str, "__ignore_typecheck__") || S_EQ(str, "return") || S_EQ(str, "include") ||
           S_EQ(str, "sizeof") || S_EQ(str, "if") || S_EQ(str, "else") || S_EQ(str, "while") ||
           S_EQ(str, "for") || S_EQ(str, "do") || S_EQ(str, "break") || S_EQ(str, "continue") ||
           S_EQ(str, "switch") || S_EQ(str, "case") || S_EQ(str, "default") || S_EQ(str, "goto") ||
           S_EQ(str, "typedef") || S_EQ(str, "const") || S_EQ(str, "extern") || S_EQ(str, "restrict");
}

static struct token *token_make_operator_or_string()
{
    char op = peekc();
    // #include<abc.h>
    if (op == '<')
    {
        struct token *last_token = lex_last_token();
        if (token_is_keyword(last_token, "include"))
            return token_make_string('<', '>');
    }
    struct token *token = token_create(&(struct token){.type = TOKEN_TYPE_OPERATOR, .svar = read_op()});
    if (op == '(')
    {
        lex_new_expression();
    }
    // ')' end
    return token;
}

struct token *token_make_one_line_comment()
{
    // hello world
    struct buffer *buff = buffer_create();
    char c = 0;
    LEX_GETC_IF(buff, c, (c != '\n' && c != EOF));
    return token_create(&(struct token){.type = TOKEN_TYPE_COMMENT, .svar = buffer_ptr(buff)});
}

struct token *token_make_multiline_comment()
{
    /*
        hello world
    */
    struct buffer *buff = buffer_create();
    char c = 0;
    // 循环去除/*******/   
    while (1)
    {
        LEX_GETC_IF(buff, c, (c != '*' && c != EOF));
        if(c == EOF)
        {
            compiler_error(lexer->compiler, "You did not close this multiline comment");
        }
        else if(c == '*')
        {
            nextc();
            if(peekc() == '/')
            {
                nextc();
                break;
            }
        }
    }
    return token_create(&(struct token){.type = TOKEN_TYPE_COMMENT, .svar = buffer_ptr(buff)});
}

struct token *handle_comment()
{
    char c = peekc();
    if(c == '/')
    {
        nextc();
        c = peekc();
        if(c == '/')
        {
            nextc();
            return token_make_one_line_comment();
        }
        else if(c == '*')
        {
            nextc();
            return token_make_multiline_comment();
        }
        // 此时为 除号 '/'
        pushc('/');
        return token_make_operator_or_string();
    }
    
    return NULL;
}

static struct token *token_make_symbol()
{
    char c = nextc();
    if (c == ')')
    {
        lex_finish_expression();
    }
    struct token *token = token_create(&(struct token){.type = TOKEN_TYPE_SYMBOL, .cvar = c});
    return token;
}

static struct token *token_make_identifier_or_keyword()
{
    struct buffer *buff = buffer_create();
    char c = 0;
    // isalnum()
    LEX_GETC_IF(buff, c, ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') || (c == '_'));
    buffer_write(buff, 0x00);

    const char *str = buffer_ptr(buff);
    // 检查是否为keyword
    if (is_keyword(str))
    {
        return token_create(&(struct token){.type = TOKEN_TYPE_KEYWORLD, .svar = str});
    }
    return token_create(&(struct token){.type = TOKEN_TYPE_IDENTIFIER, .svar = str});
}

struct token *read_special_token()
{
    char c = peekc();
    if (isalpha(c) || c == '_')
    {
        return token_make_identifier_or_keyword();
    }
    return NULL;
}

struct token *token_make_newline()
{
    nextc();
    return token_create(&(struct token){.type = TOKEN_TYPE_NEWLINE});
}

struct token *read_next_token()
{
    struct token *token = NULL;
    char c = peekc();

    token = handle_comment();
    if(token)
    {
        printf("%s ", token->svar);
        return token;
    }

    switch (c)
    {
    NUMERIC_CASE:
        token = token_make_number();
        printf("%lld ", token->llnum);
        break;
    OPERATOR_CASE_EXCLUDING_DIVISION:
        token = token_make_operator_or_string();
        printf("%s ", token->svar);
        break;
    SYMBOL_CASE:
        token = token_make_symbol();
        printf("%c ", token->cvar);
        break;
    case '"':
        token = token_make_string('"', '"');
        printf("%s ", token->svar);
        break;
    case ' ':
    case '\t':
        token = handle_whitespace();
        break;
    case '\n':
        token = token_make_newline();
        printf("\n");
        break;
    case EOF:
        // 读取结束
        break;
    default:
        token = read_special_token();
        if (!token)
        {
            compiler_error(lexer->compiler, "Unexpected token!");
        }
        printf("%s ", token->svar);
    }

    return token;
}

int lex(struct lex_process *process)
{
    process->current_expression_count = 0;
    process->parentheses_buffer = NULL;
    process->pos.filename = process->compiler->ifile.abs_path;
    lexer = process;

    struct token *token = read_next_token();
    while (token)
    {
        vector_push(lexer->token_vec, token);
        token = read_next_token();
    }
    printf("\n");
    return LEXICAL_ANALYSIS_ALL_OK;
}