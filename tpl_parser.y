/*
 * YATE template engine
 *
 * $Id: tpl_parser.y,v 1.3 2003/12/21 01:07:34 mclap Exp $
 */
%{
#include <stdio.h>
#include <string.h>
#include "yate.h"

#define YYDEBUG 1
#define YYERROR_VERBOSE 1

struct _yp_state *__ys = 0;

extern void swctx(int ctx);
extern int vfree_debug;

int yywrap() { return 1; }

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
%token IF ELSEIF ELSE IF_CLOSE
%token WHILE WHILE_CLOSE
%token INCLUDE

%type <node> block template_block expr var stmt elseif_block else_block assign_stmt

%%

template:
	template_block		{
		__ys->prog = $1;
		/*
		lexvalue *v = nexec($1);
		if (v) {
			vbuf_write(v);
			if (v->tmp) vfree(v);
		}

		nfree($1);
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
	| INCLUDE expr '}'		{ $$ = nop(INCLUDE, 1, $2); }
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
	/* empty */ { $$ = 0; }
	| elseif_block ELSEIF expr '}' { swctx(0); } template_block else_block { $$ = nop(IF, 3, $3, $6, $7); }
	;

else_block:
	/* empty */ { $$ = 0; }
	| ELSE { swctx(0); } template_block { $$ = $3; }
	;

%%

int yyerror(char *s) {
extern char *yytext;
extern int getctx();
    fprintf(stdout, "[%d:%d] %s (yytext=%s, yy_start=%d)\n", __ys->line, __ys->pos, s, yytext, getctx());
}

extern char *_outbuffer;
extern int _ob_length;
extern int _ob_size;

lnode *_yate_prepare(char *filename) {
	extern FILE *yyin;
	struct _yp_state ys, *ys_prev;
	FILE *in_prev;

	in_prev = yyin;
	yyin = fopen(filename,"r");
	if (!yyin) {
		yyin=in_prev;
		return 0;
	}

	memset(&ys, 0, sizeof(struct _yp_state));

	ys_prev = __ys; __ys = &ys;
	yyparse();
	__ys = ys_prev;

	fclose(yyin);

	yyin = in_prev;
	return ys.prog;
}
