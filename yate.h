/*
 * YATE template engine
 *
 * $Id: yate.h,v 1.1 2003/12/21 01:09:22 mclap Exp $
 */
#ifndef __yate_h__
#define __yate_h__

//#define YT_DEBUG

#define LV_NULL		0
#define LV_INT		1
#define LV_STRING	2
#define LV_FLOAT	3
#define LV_ARRAY	4

#ifdef YT_DEBUG
#define FREE(ptr)	{ printf("FREE(%08lx), %s:%d\n", (ptr), __FILE__, __LINE__); free(ptr); }
#else
#define FREE(ptr)	free(ptr);
#endif

inline void *CALLOC(size_t n, size_t size);

/* template variables */
typedef struct _lexvalue {
	int type;	/* 0 - null, 1 - int, 2 - string, 3 - float, 4 - array */
	int tmp;	/* 0 - normal, 1 - temporaly */
	union {
		int ival;
		char *sval;
		float fval;
		struct {
			size_t count;
			size_t size;
			struct _lexvalue **keys;
			struct _lexvalue **vals;
		}aval;
	}u;
} lexvalue;

#define LNODE_DATA	0
#define LNODE_OPER	1
#define LNODE_VAR	2
#define LNODE_CONST	3
#define LNODE_FUNC	3

typedef struct _lnode {
	int type;
	union {
		lexvalue *data;
		char *var;
		struct {
			int id;
			int count;
			struct _lnode **args;
		}oper;
	}u;
	lexvalue *node_value;
}lnode;

lnode *ndata(lexvalue *);
lnode *nvar(char *ptr, int len);
lnode *nop(int id, int cnt, ...);
void nfree(lnode *r);

/* template engine */
typedef struct {
	lexvalue *GLOBALS;
	struct {
		char *buf;
		int len;
		int size;
	}out;
} yate;

struct _yp_state {
	char *file;
	int line, pos;
	lnode *prog;
};

extern struct _yp_state *__ys;

yate *yate_init();
int yate_exec(yate *ts, char *filename);
void yate_free(yate *ts);
lnode * _yate_parse_stream(FILE *fp);
lnode * _yate_prepare(char *filename);
lexvalue *nexec(yate *ts, lnode *n);

//lexvalue *vnew(int type, void *data, size_t optLen);
lexvalue *vnew(int type, ...);
void vfree(lexvalue *val);
lexvalue *vdupe(lexvalue *data);
void vadd(lexvalue *arr, lexvalue *key, lexvalue *value);
size_t vfindn(lexvalue *arr, lexvalue *key);
lexvalue *vfind(lexvalue *arr, lexvalue *key);

int vbuf_write(yate *ts, lexvalue *val);
int vbuf_write2(yate *ts, char *str);
void vbuf_clear(yate *ts);

char *val2str(lexvalue *val);
int val2int(lexvalue *val);
float val2float(lexvalue *val);
int val2bool(lexvalue *val);

#endif // __yate_h__
