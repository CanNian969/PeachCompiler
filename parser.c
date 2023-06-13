#include "compiler.h"
#include "helpers/vector.h"

struct compile_process *current_compiler;
static struct token *parser_last_token;

static void parse_ignore_nl_or_comment(struct token *token)
{
    while (token && token_is_nl_or_comment_or_newline_seperator(token))
    {
        vector_peek(current_compiler->token_vec);
        token = vector_peek_no_increment(current_compiler->token_vec);
    }
}

static struct token *token_next()
{
    struct token *next_token = vector_peek_no_increment(current_compiler->token_vec);
    parse_ignore_nl_or_comment(next_token);
    current_compiler->pos = next_token->pos;
    parser_last_token = next_token;
    return vector_peek(current_compiler->token_vec);
}

static struct token *token_peek()
{
    struct token *next_token = vector_peek_no_increment(current_compiler->token_vec);
    parse_ignore_nl_or_comment(next_token);
    return next_token;
}

void *parse_single_token_to_node()
{
    struct token *token = token_next();
    struct node *node = NULL;
    switch (token->type)
    {
    case TOKEN_TYPE_NUMBER:
        node = node_create(&(struct node){.type = NODE_TYPE_NUMBER, .llnum = token->llnum});
        // printf("node number: %lld\n", node->llnum);
        // printf("node number: %c\n", token->cval);
        break;

    case TOKEN_TYPE_IDENTIFIER:
        node = node_create(&(struct node){.type = NODE_TYPE_IDENTIFIER, .sval = token->sval});
        // printf("node identifier: %s\n", node->sval); 
        break;

    case TOKEN_TYPE_STRING:
        node = node_create(&(struct node){.type = NODE_TYPE_STRING, .sval = token->sval});
        // printf("node string: %s\n", node->sval);
        break;

    default:
        compiler_error(current_compiler, "This is not a single tokne that can be converted to a node");
    }
}

int parse_next()
{
    struct token *token = token_peek();
    if (!token)
    {
        return -1;
    }
    // printf("%d\n", token->type);
    switch (token->type)
    {
    case TOKEN_TYPE_NUMBER:
    case TOKEN_TYPE_IDENTIFIER:
    case TOKEN_TYPE_STRING:
        parse_single_token_to_node();
        break;

    default:
        token_next();
        break;
    }

    return 0;
}

int parse(struct compile_process *process)
{
    current_compiler = process;
    parser_last_token = NULL;
    node_set_vector(process->node_vec, process->node_tree_vec);

    struct node *node = NULL;
    // 初始化头指针index
    vector_set_peek_pointer(process->token_vec, 0);
    // printf("%d\n", vector_count(process->token_vec));
    while (parse_next() == 0)
    {
        node = node_peek();
        vector_push(process->node_tree_vec, &node);
    }
    // printf("length\n");
    // printf("%d\n", vector_count(process->node_vec));
    // printf("%d\n", vector_count(process->node_tree_vec));
    return PARSE_ALL_OK;
}
