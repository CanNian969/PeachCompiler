#include "compiler.h"

bool token_is_keyword(struct token* token, const char* keyword)
{
    return token->type == TOKEN_TYPE_KEYWORLD && S_EQ(token->svar, keyword);
}