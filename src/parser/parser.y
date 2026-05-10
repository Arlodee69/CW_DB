%{
#include "../../include/parser/AST.hpp"
// подключения
%}

%union {
    int   ival;
    char* sval;
    ASTNode* node;
}

%token KW_SELECT KW_FROM KW_WHERE KW_INSERT KW_INTO
%token KW_UPDATE KW_SET KW_DELETE KW_CREATE KW_DROP
%token KW_DATABASE KW_TABLE KW_USE KW_VALUE
%token KW_NOT_NULL KW_INDEXED KW_AND KW_OR KW_BETWEEN KW_LIKE KW_AS KW_NULL
%token KW_INT KW_STRING
%token OP_EQ OP_NEQ OP_LT OP_GT OP_LTE OP_GTE OP_ASSIGN
%token SYM_LPAREN SYM_RPAREN SYM_COMMA SYM_SEMICOLON SYM_STAR SYM_DOT
%token <ival> LIT_INT
%token <sval> LIT_STRING IDENTIFIER

%%

statement:
    select_stmt SYM_SEMICOLON
  | insert_stmt SYM_SEMICOLON
  | update_stmt SYM_SEMICOLON
  | delete_stmt SYM_SEMICOLON
  | create_db_stmt SYM_SEMICOLON
  | drop_db_stmt SYM_SEMICOLON
  | use_stmt SYM_SEMICOLON
  | create_table_stmt SYM_SEMICOLON
  | drop_table_stmt SYM_SEMICOLON
  ;

select_stmt:
    KW_SELECT columns KW_FROM table_ref
  | KW_SELECT columns KW_FROM table_ref KW_WHERE condition
  ;

condition:
    condition KW_AND condition   /* задание 11 — уже работает */
  | condition KW_OR  condition   /* задание 11 — уже работает */
  | expr OP_EQ  expr
  | expr OP_NEQ expr
  | expr OP_LT  expr
  | expr OP_GT  expr
  | expr OP_LTE expr
  | expr OP_GTE expr
  | expr KW_BETWEEN expr KW_AND expr
  | expr KW_LIKE LIT_STRING
  | SYM_LPAREN condition SYM_RPAREN
  ;

/* ... остальные правила */

%%