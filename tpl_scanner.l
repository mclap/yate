/*
 * YATE template engine
 *
 * $Id: tpl_scanner.l,v 1.3 2003/12/20 22:44:16 mclap Exp $
 */
%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tpl_parser.h"
#include "lextpl.h"

int tpl_pos = 0;
int tpl_line = 0;

#define HANDLE_POS(ap, l)										\
{																\
	register char *p, *end;										\
	p = (ap); end = p + (l);									\
	while (p < end) {											\
		if (*p == '\n') {										\
			tpl_line++;											\
			tpl_pos = 0;										\
		}														\
		else {													\
			if (*p == '\r' && p+1 < end && *(p+1) == '\n') {	\
				tpl_line++;										\
				tpl_pos = 0;									\
			}													\
			else												\
				tpl_pos++;										\
		}														\
		p++;													\
	}															\
}

#define HPOS	HANDLE_POS(yytext, yyleng)
#define STDPOS	{ tpl_pos += yyleng; }


%}

%x IN_TEMPLATE
%x IN_LITERAL
%x IN_TPL_DQUOTE
%x IN_COMMENT

LABEL		[a-zA-Z_]+[0-9a-zA-Z_]*
SPACE		[ \t\r\n]+
NEWLINE		("\r\n"|"\r"|"\n")
INTEGER		[0-9]+
FLOAT		[0-9]*\.[0-9]+


%%

<INITIAL>\{ {	
	STDPOS;
	BEGIN(IN_TEMPLATE);
	return '{';
}

<INITIAL>"{pre}"{NEWLINE}? {
	HPOS;
	BEGIN(IN_LITERAL);
}

<INITIAL>"{*" {
	STDPOS;
	BEGIN(IN_COMMENT);
}

<INITIAL>"{if:"		{ STDPOS; BEGIN(IN_TEMPLATE); return IF; }
<INITIAL>"{elseif:"	{ STDPOS; BEGIN(IN_TEMPLATE); return ELSEIF; }
<INITIAL>"{else}"	{ STDPOS; return ELSE; }
<INITIAL>"{/if}"	{ STDPOS; return IF_CLOSE; }

<INITIAL>"{while:"	{ STDPOS; BEGIN(IN_TEMPLATE); return WHILE; }
<INITIAL>"{/while}"	{ STDPOS; return WHILE_CLOSE; }

<INITIAL>[^{]+ {
	yylval.lexval = vnew(LV_STRING, yytext, yyleng);
	HPOS;
	return DATA;
}

<IN_TEMPLATE>\} {
	STDPOS;
	BEGIN(INITIAL);
	return '}';
}

<IN_TEMPLATE>{LABEL} {
	HPOS;
	yylval.str.val = yytext;
	yylval.str.len = yyleng;
	return T_NAME;
}

<IN_TEMPLATE>{SPACE} {
	HPOS;
	/*return SPACE;*/
}

<IN_TEMPLATE>{INTEGER} {
	STDPOS;
	yylval.ival = strtol(yytext,0,10);
	return INTEGER;
}

<IN_TEMPLATE>{FLOAT} {
	STDPOS;
	yylval.fval = strtod(yytext, 0);
	return FLOAT;
}

<IN_TEMPLATE>">="|"ge"	{ STDPOS; return GE; }
<IN_TEMPLATE>"<="|"le"	{ STDPOS; return LE; }
<IN_TEMPLATE>"=="|"eq"	{ STDPOS; return EQ; }
<IN_TEMPLATE>"!="|"ne"	{ STDPOS; return NE; }

<IN_TEMPLATE>\"			{ STDPOS; BEGIN(IN_TPL_DQUOTE); }

<IN_TEMPLATE>.			{ STDPOS; return *yytext; }

<IN_TPL_DQUOTE>[^"]+	yymore();

<IN_TPL_DQUOTE>\"		{
	HPOS;
	yylval.lexval = vnew(LV_STRING, yytext, yyleng-1);
	BEGIN(IN_TEMPLATE);
	return STRING;
}

<IN_LITERAL>"{/pre}"	{
	HPOS;
	yylval.lexval = vnew(LV_STRING, yytext, yyleng-6);
	BEGIN(INITIAL);
	return DATA;
}

<IN_LITERAL>[^{]+ {
	yymore();
}

<IN_LITERAL>"{" {
	yymore();
}

<IN_COMMENT>[^*]+ {
	yymore();
}

<IN_COMMENT>"*}"{NEWLINE}? {
	HPOS;
	BEGIN(INITIAL);
}

%%

void swctx(int ctx) {
//	printf("[%d:%d] swctx(%d)\n", tpl_line, tpl_pos, ctx);
	if (ctx == 0) { BEGIN(INITIAL); }
	else { BEGIN(IN_TEMPLATE); }
}


int getctx() { return yy_start; }
