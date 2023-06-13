#include "compiler.h"

bool token_is_keyword(struct token *token, const char *keyword)
{
    return token->type == TOKEN_TYPE_KEYWORLD && S_EQ(token->sval, keyword);
}

bool token_is_symbol(struct token *token, char c)
{
    return token->type == TOKEN_TYPE_SYMBOL && token->cval == c;
}

bool token_is_nl_or_comment_or_newline_seperator(struct token *token)
{
    return token->type == TOKEN_TYPE_NEWLINE ||
           token->type == TOKEN_TYPE_COMMENT ||
           token_is_symbol(token, '\\');
}
