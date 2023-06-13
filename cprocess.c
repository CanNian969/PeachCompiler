#include "compiler.h"
#include <stdarg.h>
#include "helpers/vector.h"

void compiler_error(struct compile_process *cprocess, const char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
    fprintf(stderr, " on line %i, on col %i in file %s\n", cprocess->pos.line, cprocess->pos.col, cprocess->pos.filename);
    exit(-1);
}

void compiler_warning(struct compile_process *cprocess, const char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
    fprintf(stderr, " on line %i, on col %i in file %s\n", cprocess->pos.line, cprocess->pos.col, cprocess->pos.filename);
}

struct compile_process *compile_process_create(const char *filename, const char *out_filename, int flags)
{
    FILE *infile = fopen(filename, "r");
    if (!infile)
    {
        return NULL;
    }
    FILE *outfile = NULL;
    if (out_filename)
    {
        outfile = fopen(out_filename, "w");
        if (!outfile)
            return NULL;
    }

    struct compile_process *process = calloc(1, sizeof(struct compile_process));
    process->node_vec = vector_create(sizeof(struct node *));
    process->node_tree_vec = vector_create(sizeof(struct node *));

    process->flags = flags;
    process->pos.line = 1;
    process->pos.col = 1;
    process->ifile.fp = infile;
    process->ifile.abs_path = filename;
    process->ofile = outfile;
    return process;
}

char compile_process_next_char(struct lex_process *lexer)
{
    struct compile_process *compiler = lexer->compiler;
    compiler->pos.col += 1;

    char c = getc(compiler->ifile.fp);
    if (c == '\n')
    {
        compiler->pos.line += 1;
        compiler->pos.col = 1;
    }
    return c;
};

char compile_process_peek_char(struct lex_process *lexer)
{
    struct compile_process *compiler = lexer->compiler;
    char c = getc(compiler->ifile.fp);
    ungetc(c, compiler->ifile.fp);
    return c;
};

void compile_process_push_char(struct lex_process *lexer, char c)
{
    struct compile_process *compiler = lexer->compiler;
    ungetc(c, compiler->ifile.fp);
    return;
};
