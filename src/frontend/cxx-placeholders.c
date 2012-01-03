#include "cxx-placeholders.h"
#include "cxx-process.h"

#include <string.h>
#include <stdio.h>

#define TOK_SEPARATOR "::"
static AST* decode_placeholder(const char *c)
{
    const char * colons = strstr(c, TOK_SEPARATOR);

    if (colons == NULL)
    {
        internal_error("Invalid placeholder token", 0);
    }

    colons += strlen(TOK_SEPARATOR);

    AST *tree = NULL;
    sscanf(colons, "%p", &tree);

    if (tree == NULL)
    {
        internal_error("Invalid AST* reference", 0);
    }

    return tree;
}

void check_statement_placeholder(AST placeholder, decl_context_t decl_context UNUSED_PARAMETER, nodecl_t* nodecl_output)
{
    AST* p = decode_placeholder(ASTText(placeholder));

    nodecl_t empty_stmt = nodecl_make_empty_statement(ASTFileName(placeholder), ASTLine(placeholder));

    *p = nodecl_get_ast(empty_stmt);

    *nodecl_output = nodecl_make_list_1(_nodecl_wrap(*p));
}