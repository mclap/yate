/*
 * YATE template engine
 *
 * $Id: tpl_parser.y,v 1.2 2003/12/20 22:34:40 mclap Exp $
 */
%{
#include <stdio.h>
#include <string.h>
#include "lextpl.h"

#define YYDEBUG 1
#define YYERROR_VERBOSE 1

#define YY_parse_PARSE_PARAM lte
#define YY_parse_PARSE_PARAM_DEF lextpl *lte

lexvalue *GLOBALS;

extern int tpl_pos, tpl_line;

extern void swctx(int ctx);
extern int vfree_debug;

int yywrap() { return 1; }

#define TPL	1

%}

%union {
	struct {
		char *val;
		int len;
	} str;
	float fval;
	int ival;
	struct _lexvalue *lexval;
	struct _lnode *node;
};

%token SPACE
%token <lexval> DATA
%token <str> T_NAME
%token <ival> INTEGER
%token <fval> FLOAT
%token <lexval> STRING

%left '.'
%left GE LE EQ NE '>' '<'
%left '+' '-'
%left '*' '/'
%left '['
%nonassoc UMINUS
%token IF
%token ELSEIF ELSE IF_CLOSE
%token WHILE WHILE_CLOSE

%type <node> block template_block expr var stmt elseif_block else_block assign_stmt

%%

template:
	template_block		{
		lexvalue *v = nexec($1);
		if (v) {
			vbuf_write(v);
			if (v->tmp) vfree(v);
		}
/*printf("\n===============\nbefore nfree\n");
	getch();
vfree_debug=1;
*/
		nfree($1);
/*
printf("\n===============\nafter nfree\n");
	getch();
*/
	}
	;

template_block:
	block					{ $$ = $1; }
	| template_block block	{ $$ = nop(';',2, $1,$2); }
	;

block:
	DATA					{ $$ = ndata($1); }
	| IF expr '}' { swctx(0); } template_block elseif_block IF_CLOSE { $$ = nop(IF, 3, $2, $5, $6); }
	| WHILE expr '}' { swctx(0); } template_block WHILE_CLOSE { $$ = nop(WHILE, 2, $2, $5); }
	| '{' stmt '}' { $$ = $2; }
	;

stmt:
	/* empty */				{ $$ = 0; }
	| stmt ';' stmt			{ $$ = nop(';', 2, $1, $3); }
	| expr					{ $$ = $1; }
	| assign_stmt			{ $$ = $1; }
	;

assign_stmt:
	var '=' expr			{ $$ = nop('=', 2, $1, $3); }
	;
	
var:
	'$' T_NAME				{ $$ = nvar($2.val, $2.len); }
	;

expr:
	var						{ $$ = $1; }
	| INTEGER				{ $$ = ndata(vnew(LV_INT, $1)); }
	| FLOAT					{ $$ = ndata(vnew(LV_FLOAT, $1)); }
	| STRING				{ $$ = ndata($1); }
	| '-' expr %prec UMINUS	{ $$ = nop(UMINUS, 1, $2); }
	| expr '+' expr			{ $$ = nop('+', 2, $1, $3); }
	| expr '-' expr			{ $$ = nop('-', 2, $1, $3); }
	| expr '*' expr			{ $$ = nop('*', 2, $1, $3); }
	| expr '/' expr			{ $$ = nop('/', 2, $1, $3); }
	| expr '>' expr			{ $$ = nop('>', 2, $1, $3); }
	| expr '<' expr			{ $$ = nop('<', 2, $1, $3); }
	| expr '.' expr			{ $$ = nop('.', 2, $1, $3); }
	| expr GE expr			{ $$ = nop(GE, 2, $1, $3); }
	| expr LE expr			{ $$ = nop(LE, 2, $1, $3); }
	| expr NE expr			{ $$ = nop(NE, 2, $1, $3); }
	| expr EQ expr			{ $$ = nop(EQ, 2, $1, $3); }
	| '(' expr ')'			{ $$ = $2; }
	| expr '[' expr ']'		{ $$ = nop('[', 2, $1, $3); }
	;

elseif_block:
	/* empty */ { $$ = 0; printf("{0}"); }
	| elseif_block ELSEIF expr '}' { swctx(0); printf("["); } template_block else_block { $$ = nop(IF, 3, $3, $6, $7); }
	;

else_block:
	/* empty */ { $$ = 0; }
	| ELSE { swctx(0); printf("["); } template_block { $$ = $3; }
	;

%%

int yyerror(char *s) {
extern char *yytext;
extern int getctx();
    fprintf(stdout, "[%d:%d] %s (yytext=%s, yy_start=%d)\n", tpl_line, tpl_pos, s, yytext, getctx());
}

extern char *_outbuffer;
extern int _ob_length;
extern int _ob_size;

int main() {
	lexvalue *k, *v;
	extern FILE *yyin;

//	yyin = fopen("test_current","r");
/*
printf("before start\n");
	getch();
*/
	GLOBALS = vnew(LV_ARRAY);

/*
	k = vnew(LV_STRING,"world",0);
	v = vnew(LV_STRING,"big world",0);
	vadd(GLOBALS, k, v);

	k = vnew(LV_STRING,"i",0); v = vnew(LV_STRING,"2",0); vadd(GLOBALS, k, v);
	k = vnew(LV_STRING,"j",0); v = vnew(LV_STRING,"7",0); vadd(GLOBALS, k, v);

	vfree(GLOBALS);
*/
	yyparse();
	printf("\n===============\nsizeof ob=%d\n----------\n", _ob_length);
	printf("%s\n", _outbuffer);
/*
	getch();
*/
	FREE(_outbuffer);
	vfree(GLOBALS);
/*
*/
//	getch();
	return 0;
}
