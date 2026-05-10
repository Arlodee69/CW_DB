%{
#include "parser.tab.hpp"   // сгенерированный Bison заголовок
%}

%%

[ \t\r]+        { /* пропускаем пробелы */ }
\n              { yylineno++; }
"--"[^\n]*      { /* пропускаем комментарии */ }

[Ss][Ee][Ll][Ee][Cc][Tt]   { return KW_SELECT; }
[Ff][Rr][Oo][Mm]           { return KW_FROM; }
[Ww][Hh][Ee][Rr][Ee]       { return KW_WHERE; }
[Ii][Nn][Ss][Ee][Rr][Tt]   { return KW_INSERT; }
"int"                       { return KW_INT; }
"string"                    { return KW_STRING; }
"NOT_NULL"                  { return KW_NOT_NULL; }
"INDEXED"                   { return KW_INDEXED; }
"AND"                       { return KW_AND; }
"OR"                        { return KW_OR; }
"BETWEEN"                   { return KW_BETWEEN; }
"LIKE"                      { return KW_LIKE; }
"NULL"                      { return KW_NULL; }

[0-9]+          { yylval.ival = atoi(yytext); return LIT_INT; }
\"[^\"]*\"      { yylval.sval = strdup(yytext+1); /* убираем кавычки */
                  yylval.sval[strlen(yylval.sval)-1] = '\0';
                  return LIT_STRING; }

[a-zA-Z_][a-zA-Z0-9_]*  { yylval.sval = strdup(yytext); return IDENTIFIER; }

"=="  { return OP_EQ; }
"!="  { return OP_NEQ; }
"<="  { return OP_LTE; }
">="  { return OP_GTE; }
"<"   { return OP_LT; }
">"   { return OP_GT; }
"="   { return OP_ASSIGN; }
"("   { return SYM_LPAREN; }
")"   { return SYM_RPAREN; }
","   { return SYM_COMMA; }
";"   { return SYM_SEMICOLON; }
"*"   { return SYM_STAR; }
"."   { return SYM_DOT; }

.     { /* неизвестный символ — ошибка */ }

%%