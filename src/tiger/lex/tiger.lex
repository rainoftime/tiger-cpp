%{
#include <string>
#include <iostream>
#include "parse.tab.hh"
#include "tiger/errormsg/errormsg.h"
#include "tiger/symbol/symbol.h"

static int comment_level = 0;
static std::string string_buf;
static int char_pos = 1;
static err::ErrorMsg *errormsg;

void adjust() {
    errormsg->SetTokPos(char_pos);
    char_pos += yyleng;
}

void adjustStr() {
    char_pos += yyleng;
}

void adjustIgn() {
    char_pos += yyleng;
}



%}

%option noyywrap
%option yylineno

 /* definitions */
letter [A-Za-z]
digits [0-9]+
printable \\[0-9]{3}
control \\\^[A-Z]

%x COMMENT STR IGNORE

%%

 /* operators */
"," { adjust(); return COMMA; }
":" { adjust(); return COLON; }
";" { adjust(); return SEMICOLON; }
"(" { adjust(); return LPAREN; }
")" { adjust(); return RPAREN; }
"[" { adjust(); return LBRACK; }
"]" { adjust(); return RBRACK; }
"{" { adjust(); return LBRACE; }
"}" { adjust(); return RBRACE; }
"." { adjust(); return DOT; }
"+" { adjust(); return PLUS; }
"-" { adjust(); return MINUS; }
"*" { adjust(); return TIMES; }
"/" { adjust(); return DIVIDE; }
"=" { adjust(); return EQ; }
"<" { adjust(); return LT; }
">" { adjust(); return GT; }
"&" { adjust(); return AND; }
"|" { adjust(); return OR; }
":=" { adjust(); return ASSIGN; }
"<>" { adjust(); return NEQ; }
"<=" { adjust(); return LE; }
">=" { adjust(); return GE; }

 /* reserved words */
"array" { adjust(); return ARRAY; }
"if" { adjust(); return IF; }
"then" { adjust(); return THEN; }
"else" { adjust(); return ELSE; }
"while" { adjust(); return WHILE; }
"for" { adjust(); return FOR; }
"to" { adjust(); return TO; }
"do" { adjust(); return DO; }
"let" { adjust(); return LET; }
"in" { adjust(); return IN; }
"end" { adjust(); return END; }
"of" { adjust(); return OF; }
"break" { adjust(); return BREAK; }
"nil" { adjust(); return NIL; }
"function" { adjust(); return FUNCTION; }
"var" { adjust(); return VAR; }
"type" { adjust(); return TYPE; }

 /* literals */
{letter}[A-Za-z0-9_]* { 
    adjust(); 
    yylval.sym = sym::Symbol::UniqueSymbol(std::string(yytext, yyleng));
    return ID; 
}
{digits} { 
    adjust(); 
    yylval.ival = std::stoi(yytext);
    return INT; 
}

 /* strings */
\" { adjust(); BEGIN(STR); string_buf.clear(); }
<STR>\" { 
    adjustStr(); 
    BEGIN(INITIAL); 
    yylval.sval = new std::string(string_buf);
    return STRING; 
}
<STR>\\n { adjustStr(); string_buf += '\n'; }
<STR>\\t { adjustStr(); string_buf += '\t'; }
<STR>\\\" { adjustStr(); string_buf += '\"'; }
<STR>\\\\ { adjustStr(); string_buf += '\\'; }
<STR>{control} { 
    adjustStr(); 
    string_buf += (char)(yytext[2] - 'A' + 1); 
}
<STR>{printable} {
    adjustStr();
    int pChar;
    sscanf(yytext, "\\%d", &pChar);
    string_buf += (char)pChar;
}
<STR>\\ { adjustIgn(); BEGIN(IGNORE); }
<STR>. { adjustStr(); string_buf += yytext[0]; }
<STR><<EOF>> { errormsg->Error(errormsg->GetTokPos(), "unterminated string"); }

<IGNORE>[\n\t ] { adjustIgn(); }
<IGNORE>\\ { adjustIgn(); BEGIN(STR); }

 /* comments */
<INITIAL,COMMENT>"/*" { 
    adjust(); 
    comment_level++; 
    BEGIN(COMMENT); 
}
<COMMENT>"*/" {
    adjust();
    comment_level--;
    if (comment_level == 0)
        BEGIN(INITIAL);
}
<COMMENT>\n { adjust(); errormsg->Newline(); }
<COMMENT>. { adjust(); }
<COMMENT><<EOF>> { errormsg->Error(errormsg->GetTokPos(), "unterminated comment"); }

 /*
  * skip white space chars.
  * space, tabs and LF
  */
[ \t]+ { adjust(); }
\n { adjust(); errormsg->Newline(); }

 /* illegal input */
. { adjust(); errormsg->Error(errormsg->GetTokPos(), "illegal token"); }

%%

void InitLexer(err::ErrorMsg *error_msg) {
    errormsg = error_msg;
    char_pos = 1;
    comment_level = 0;
}
