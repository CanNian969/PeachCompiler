#ifndef COMPILER_H
#define COMPILER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// macro's make life cleaner
#define S_EQ(str, str2) \
    (str && str2 && (strcmp(str, str2) == 0))

#define NUMERIC_CASE \
    case '0':        \
    case '1':        \
    case '2':        \
    case '3':        \
    case '4':        \
    case '5':        \
    case '6':        \
    case '7':        \
    case '8':        \
    case '9'

#define OPERATOR_CASE_EXCLUDING_DIVISION \
    case '+':                            \
    case '-':                            \
    case '*':                            \
    case '<':                            \
    case '>':                            \
    case '^':                            \
    case '%':                            \
    case '!':                            \
    case '=':                            \
    case '~':                            \
    case '|':                            \
    case '&':                            \
    case '(':                            \
    case '[':                            \
    case ',':                            \
    case '.':                            \
    case '?'

#define SYMBOL_CASE \
    case '{':       \
    case '}':       \
    case ':':       \
    case ';':       \
    case '#':       \
    case '\\':      \
    case ')':       \
    case ']'

enum
{
    COMPILER_FILE_COMPILED_OK,
    COMPILER_FAILED_WITH_ERROR
};

enum
{
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_SYMBOL,
    TOKEN_TYPE_OPERATOR,
    TOKEN_TYPE_KEYWORLD,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_COMMENT,
    TOKEN_TYPE_NEWLINE
};

enum
{
    LEXICAL_ANALYSIS_ALL_OK,
    LEXICAL_ANALYSIS_INPUT_ERROR
};

struct pos
{
    int line;
    int col;
    // 记录token/process是哪个头文件里的
    const char *filename;
};

struct compile_process
{
    // 标记文件该如何编译
    int flags;
    struct pos pos;
    // 记录input file
    struct compile_process_input_file
    {
        FILE *fp;
        const char *abs_path;
    } ifile;
    // A vector of tokens from lexical analysis
    struct vector *token_vec;
    // outfile
    FILE *ofile;
};

enum
{
    NUMBER_TYPE_NORMAL,
    NUMBER_TYPE_LONG,
    NUMBER_TYPE_FLOAT,
    NUMBER_TYPE_DOUBLE
};

struct token
{
    int type;
    int flags;
    struct pos pos;

    union
    {
        char cvar;
        const char *svar;
        unsigned int inum;
        unsigned long lnum;
        unsigned long long llnum;
        void *any;
    };

    struct token_number
    {
        // 123L; for long
        int type;
    } num;

    //  与下一个token之间是否有空格，eg: * a -> operator token *和a之间
    bool whitespace;

    // eg: {hello world}  {1+2+3} {}括号之间的string
    const char *between_brackets;
};

// struct compile_process;
struct lex_process;
typedef char (*LEX_PROCESS_NEXT_CHAR)(struct lex_process *lexer);
typedef char (*LEX_PROCESS_PEEK_CHAR)(struct lex_process *lexer);
typedef void (*LEX_PROCESS_PUSH_CHAR)(struct lex_process *lexer, char c);

struct lex_process_functions
{
    LEX_PROCESS_NEXT_CHAR next_char;
    LEX_PROCESS_PEEK_CHAR peek_char;
    LEX_PROCESS_PUSH_CHAR push_char;
};

struct lex_process
{
    struct pos pos;
    struct compile_process *compiler;
    struct vector *token_vec;

    // ((50))   later explain
    int current_expression_count;
    // 括号buffer
    struct buffer *parentheses_buffer;
    struct lex_process_functions *function;

    // 使用者知道而lex不知道的私人变量
    void *lex_private;
};

// compiler.c
int compile_file(const char *filename, const char *out_filename, int flags);

// cprocess.c
void compiler_error(struct compile_process *cprocess, const char *msg, ...);
void compiler_warning(struct compile_process *cprocess, const char *msg, ...);
struct compile_process *compile_process_create(const char *filename, const char *out_filename, int flags);

char compile_process_next_char(struct lex_process *lexer);
char compile_process_peek_char(struct lex_process *lexer);
void compile_process_push_char(struct lex_process *lexer, char c);

// lex_process.c
struct lex_process *lex_process_create(struct compile_process *compiler, struct lex_process_functions *function, void *lex_private);
void lex_process_free(struct lex_process *lexer);
void *lex_process_private(struct lex_process *lexer);
struct vector *lex_process_tokens(struct lex_process *lexer);

// lexer.c
int lex(struct lex_process *process);
/**
 * @brief 从字符串中构造token
*/
struct lex_process *token_build_for_string(struct compile_process *compiler, const char *str);

// token.c
bool token_is_keyword(struct token *token, const char *keyword);

#endif