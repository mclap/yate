#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "tpl_parser.h"
#include "lextpl.h"

extern lexvalue *GLOBALS;
int vfree_debug = 0;

inline void *CALLOC(size_t n, size_t size) {
	void *r = calloc(n,size);
#ifdef YT_DEBUG
	printf("CALLOC(%08lx)\n", r);
#endif
	return r;
}

#define LEXVAL_BUFSZ	256

#define MAX_STRSZ(lv)	(((lv)&&(lv)->u.sval)?((lv)->type==LV_STRING?strlen((lv)->u.sval):LEXVAL_BUFSZ):0)

//#define VFREE(v) { printf("call vfree at line %d (%08lx): ", __LINE__, (v)); vfree(v); }
#define VFREE(v) vfree(v)

/* execution control functions */

// {{{ lnode *ndata(lexvalue *v)

lnode *ndata(lexvalue *v) {
	lnode *r;

	if (!v) return 0;

	r = (lnode *)CALLOC(1,sizeof(lnode));
	if (!r) yyerror("out of memory");

	r->type = LNODE_DATA;
	r->u.data = v;

	return r;
}

// }}}

// {{{ lnode *nvar(char *ptr, int len)

lnode *nvar(char *ptr, int len) {
	lnode *r;
	lexvalue *k;
		
	/*
	r = ndata(ptr,len);
	if (r) r->type=LNODE_VAR;
	*/
	k = vnew(LV_STRING, ptr, len);
	r = ndata(k);
	if (r) r->type=LNODE_VAR;
	
	return r;
};

// }}}

// {{{ lnode *nop(int id, int cnt, ...)

lnode *nop(int id, int cnt, ...) {
	va_list ap;
	lnode *r;
	int i;	

	r = (lnode *)CALLOC(1, sizeof(lnode));
	if (!r) yyerror("out of memory");

	r->type = LNODE_OPER;
	r->u.oper.id = id;
	r->u.oper.count = cnt;
	r->u.oper.args = (lnode **)CALLOC(cnt,sizeof(lnode *));
	if (!r->u.oper.args) yyerror("out of memory");

	va_start(ap, cnt);
	for (i=0; i<cnt; i++) r->u.oper.args[i] = va_arg(ap, lnode *);
	va_end(ap);

	return r;
};

// }}}

// {{{ void nfree(lnode *r)

void nfree(lnode *r) {
	int i;

	if (!r) return;
#ifdef YT_DENUG
	printf("nfree(%08lx/%d)\n", r, r->type);
#endif
	switch (r->type) {
		case LNODE_DATA:
		case LNODE_VAR:
			vfree(r->u.data);
			break;

		case LNODE_OPER:
			for (i=0; i<r->u.oper.count; i++)
				nfree(r->u.oper.args[i]);
			FREE(r->u.oper.args);
			break;
	};

	FREE(r);
};

// }}}

// {{{ int nexec(lnode *n, int level)

#define SIMPLE_OP(a,b,op) \
	switch (op) { \
		case '+': (a) = (a) + (b); break; \
		case '-': (a) = (a) - (b); break; \
		case '*': (a) = (a) * (b); break; \
		case '/': (a) = (a) / (b); break; \
		case '>': (a) = (a) > (b); break; \
		case '<': (a) = (a) < (b); break; \
		case GE : (a) = (a) >= (b); break; \
		case LE : (a) = (a) <= (b); break; \
		case NE : (a) = (a) != (b); break; \
		case EQ : (a) = (a) == (b); break; \
	}

lexvalue *nexec(lnode *n) {
	register int i;
	size_t j;
	register lexvalue *k, *v, *r;
	float vf;
	int vi;
	char *vs;

	if (!n) return 0;

/*
	printf("	nexec: %d >> ", n->type);
	if (n->type==LNODE_OPER)
		printf(" %d [%c]", n->u.oper.id, (char)(n->u.oper.id>=' '?n->u.oper.id:'?'));
	printf("\n");
*/
	r = k = v = 0;
	switch(n->type) {
		case LNODE_DATA:
			r = n->u.data;
//			printf(" DT[%08lx,%08lx]\n", n, n->data);
			break;

		case LNODE_OPER:
			switch (n->u.oper.id) {
				case ';':
					//printf("   [%08lx;%08lx] <= %08lx\n", n->oper.args[0], n->oper.args[1], n);
					v = nexec(n->u.oper.args[0]);
					if (v) {
						vbuf_write(v);
						if (v->tmp) vfree(v);
						v = 0;
					}
					r = nexec(n->u.oper.args[1]);
					break;

				case IF:
					k = nexec(n->u.oper.args[0]);
					if (val2bool(k)) {
						r = nexec(n->u.oper.args[1]);
					}
					else {
						if (n->u.oper.count>2)
							r = nexec(n->u.oper.args[2]);
					}
					break;

				case WHILE:
					do {
//						printf("WHILE_BEFORE_EXPR [%08lx]\n", n->u.oper.args[0]);
						k = nexec(n->u.oper.args[0]);
//						printf("WHILE_AFTER_EXPR (%08lx/%d)\n", k, k->tmp);
						i = val2bool(k);
						if (k && k->tmp) VFREE(k);

						if (i) {
							r = nexec(n->u.oper.args[1]);
							vbuf_write(r);
							if (r && r->tmp) VFREE(r);
						}
					} while(i);

					k = r = 0;

					break;

				case '=':
					i = vfindn(GLOBALS,n->u.oper.args[0]->u.data);
					v = nexec(n->u.oper.args[1]);
					if (v && !v->tmp) v = vdupe(v);
						
					v->tmp = 0;
					if (i != (size_t)-1) {
//						printf("assign [%d]%08lx=%08lx", i, GLOBALS->u.aval.vals[i], v);
						VFREE(GLOBALS->u.aval.vals[i]);
						GLOBALS->u.aval.vals[i] = v;
//						printf(", %08lx\n", GLOBALS->u.aval.vals[i]);
					}
					else {
						k = vdupe(n->u.oper.args[0]->u.data);
						vadd(GLOBALS, k, v);
					}
					k = v = 0;

					break;

				default:
					//
					// arithmetic and logic operators
					//
					if (n->u.oper.count !=2)
						break;

					k = nexec(n->u.oper.args[0]);
					v = nexec(n->u.oper.args[1]);
					if (!k || !v) {
//						printf("ERROR! [%08lx/%08lx]\n",k,v);
						break;
					}

					// strings concatenation
					if (n->u.oper.id == '.') {
						int l1, l2;
						l1 = MAX_STRSZ(k);
						l2 = MAX_STRSZ(v);
						if (l1 + l2) {
							r = vnew(LV_STRING,0);
							r->tmp = 1;
							if (r) {
								r->u.sval = (char *)CALLOC(1, l1+l2+1);
								if (l1) memcpy(r->u.sval, val2str(k), l1);
								if (l2) memcpy(r->u.sval+l1, val2str(v), l2);
								r->u.sval[l1+l2] = 0;
							}
						}

						break;
					}

					// string dedicated processing
					i = n->u.oper.id;
					if (k->type==LV_STRING && 
						(
						 i =='>' ||
						 i =='<' ||
						 i ==GE ||
						 i ==LE ||
						 i ==NE ||
						 i ==EQ
						 ) )
					{
						vi = 0;
						switch(i) {
							case '>': vi = strcmp(k->u.sval, val2str(v)) > 0; break;
							case '<': vi = strcmp(k->u.sval, val2str(v)) < 0; break;
							case GE: vi = strcmp(k->u.sval, val2str(v)) >= 0; break;
							case LE: vi = strcmp(k->u.sval, val2str(v)) <= 0; break;
							case NE: vi = strcmp(k->u.sval, val2str(v)) != 0; break;
							case EQ: vi = strcmp(k->u.sval, val2str(v)) == 0; break;
						}

						r = vnew(LV_INT, vi);
						if (r) r->tmp = 1;

						break;
					}

					// prepare left part
					switch(k->type) {
						case LV_INT:
							r = vnew(LV_INT, k->u.ival);
							break;

						case LV_FLOAT:
							r = vnew(LV_FLOAT, k->u.fval);
							break;
							
						case LV_STRING:		
							vi = val2int(k); vf = val2float(k);
							if ((float)vi != vf) r = vnew(LV_FLOAT, vf);
							else r = vnew(LV_INT, vi);
							break;

						case LV_NULL:
						case LV_ARRAY:
							r = vnew(LV_INT, 0);
							break;
					}

					if (r) r->tmp = 1;

					switch(r->type) {
						case LV_INT:
							SIMPLE_OP(r->u.ival, val2int(v), n->u.oper.id);
							break;
						case LV_FLOAT:
							SIMPLE_OP(r->u.fval, val2float(v), n->u.oper.id);
							break;
					}

					break; /* + */
			};
			break;

		case LNODE_VAR:
			r = vfind(GLOBALS,n->u.data);
//			printf(" VR[%08lx;%08lx]\n", n->data,r);
			break;
	}

	// garbage collection
/*	printf("	nexec: %d", n->type);
	if (n->type==LNODE_OPER)
		printf(" %d [%c]", n->u.oper.id, (char)(n->u.oper.id>=' '?n->u.oper.id:'?'));
	printf(" [%08lx/%d]", k, k?k->tmp:0);
	printf(" [%08lx/%d]\n", r, r?r->tmp:0);
*/
	if (k && k->tmp) VFREE(k);
	if (v && v->tmp) VFREE(v);

	return r;
}

#undef SIMPLE_OP

// }}}

/* variables management functions */

// {{{ lexvalue *vnew(int type, ...)

lexvalue *vnew(int type, ...) {
	lexvalue *r = 0;
	int l;
	va_list ap;
	char *data;

	r = (lexvalue *)CALLOC(1,sizeof(lexvalue));
	if (!r) return 0;

	r->type = type;
	va_start(ap, type);
//	if (!data && (type==LV_INT || type==LV_FLOAT || type==LV_STRING))
//		r->type = LV_NULL;

	switch (r->type) {
		case LV_INT:
			r->u.ival = va_arg(ap, int);
			break;
		case LV_STRING:
			data = va_arg(ap, char *);
			if (data) {
				l = va_arg(ap, int);
				if (l<1) l = strlen(data);

				r->u.sval = (char *)CALLOC(1,l+1);
				memcpy(r->u.sval, data, l);
				r->u.sval[l] = 0;
			}
			break;
		case LV_FLOAT:
			r->u.fval = (float)va_arg(ap, double);
			break;
		case LV_ARRAY:
			/* incomplete... */
			break;
	}

	return r;
}

// }}}

// {{{ void vfree(lexvalue *val)

void vfree(lexvalue *val) {
	int i;
	
	if (!val) return;

if(vfree_debug)	
	printf("vfree(%08lx/%d/%d)\n", val, val->tmp, val->type);

	switch (val->type) {
		case LV_STRING:
			if (val->u.sval) FREE(val->u.sval);
			break;

		case LV_ARRAY:
			for (i=0; i<val->u.aval.count; i++) {
				vfree(val->u.aval.keys[i]);
				vfree(val->u.aval.vals[i]);
			}
			FREE(val->u.aval.keys);
			FREE(val->u.aval.vals);
			break;
	}
	FREE(val);
}

// }}}

// {{{ lexvalue *vdupe(lexvalue *data)

lexvalue *vdupe(lexvalue *data) {
	lexvalue *r;
	if (!data) return 0;

	r = (lexvalue *)CALLOC(1,sizeof(lexvalue));
	if (!r) return 0;

	r->type = data->type;
	switch (r->type) {
		case LV_INT:
			r->u.ival = data->u.ival; break;

		case LV_FLOAT:
			r->u.fval = data->u.fval; break;

		case LV_STRING:
			r->u.sval = (char *)strdup(data->u.sval); break;

		case LV_ARRAY:
			r->type=LV_NULL; // FIXME: incomplete
			break;
	}
	return r;
}

// }}}

// {{{ char *val2str(lexvalue *val)

char *val2str(lexvalue *val) {
	static char buf[LEXVAL_BUFSZ];

	if (!val || val->type==LV_NULL) return 0;

	buf[sizeof(buf)-1] = 0;
	switch (val->type) {
		case LV_INT:
			snprintf(buf,sizeof(buf)-1,"%d",val->u.ival);
			break;
		case LV_FLOAT:
			snprintf(buf,sizeof(buf)-1,"%f",val->u.fval);
			break;
		case LV_STRING:
			return val->u.sval;
		case LV_ARRAY:
			strncpy(buf,"Array",sizeof(buf)-1);
		default:
			buf[0] = 0;
	}

	return buf;
}

// }}}

// {{{ int val2int(lexvalue *val)

int val2int(lexvalue *val) {
	int r;

	if (!val || val->type==LV_NULL) return 0;

	switch (val->type) {
		case LV_INT:
			r = val->u.ival;
			break;

		case LV_FLOAT:
			r = (int)val->u.fval;
			break;

		case LV_STRING:
			r = strtol(val->u.sval, 0, 10);
			break;

		default:
			r = 0;
	}

	return r;
}

// }}}

// {{{ float val2float(lexvalue *val)

float val2float(lexvalue *val) {
	float r;

	if (!val || val->type==LV_NULL) return 0;

	switch (val->type) {
		case LV_FLOAT:
			r = val->u.fval;
			break;

		case LV_INT:
			r = (float)val->u.ival;
			break;

		case LV_STRING:
			r = strtod(val->u.sval, 0);
			break;

		default:
			r = 0;
	}

	return r;
}

// }}}

// {{{ int val2bool(lexvalue *v)

int val2bool(lexvalue *v) {
	if (!v || v->type==LV_NULL) return 0;

	switch (v->type){
		case LV_ARRAY:
			return v->u.aval.count>0;
			
		case LV_INT:
			return v->u.ival != 0;

		case LV_FLOAT:
			return v->u.fval != 0;

		case LV_STRING:
			return v->u.sval!=0 && v->u.sval[0] !=0;
	}

	return 0;
}

// }}}

// {{{ size_t vfindn(lexvalue *arr, lexvalue *key)

size_t vfindn(lexvalue *arr, lexvalue *key) {
	static char buf[LEXVAL_BUFSZ];
	register size_t i;
	register lexvalue **p;
	char *bufptr = buf;
	

	if (!arr || !key || key->type==LV_ARRAY) return -1;

	if (key->type==LV_STRING) bufptr = key->u.sval;
	else strcpy(buf, val2str(key));

	for (i = 0, p = arr->u.aval.keys; i < arr->u.aval.count; i++, p++) {
		if ((*p)->type == key->type) {
			switch ((*p)->type) {
				case LV_NULL:
					return i;

				case LV_INT:
					if ((*p)->u.ival == key->u.ival)
						return i;
					break;

				case LV_FLOAT:
					if ((*p)->u.fval == key->u.fval)
						return i;
					break;

				case LV_STRING:
					if (!strcmp((*p)->u.sval, key->u.sval))
						return i;
					break;
			}
		}
		else { /* cast to string */
			if (!strcmp(bufptr,val2str(*p))) return i;
		}
	}

	return -1;
}

// }}}

// {{{ lexvalue *vfind(lexvalue *arr, lexvalue *key)

lexvalue *vfind(lexvalue *arr, lexvalue *key) {
	size_t i;

	if (!arr || !key) return 0;

	i = vfindn(arr, key);
	
	if (i != (size_t)-1) return arr->u.aval.vals[i];

	return 0;
}

// }}}

// {{{ void vadd(lexvalue *arr, lexvalue *key, lexvalue *value)

void vadd(lexvalue *arr, lexvalue *key, lexvalue *value) {
	int i=-1;
	
	if (!arr || !value) return;

	if (key) i = vfindn(arr, key);

	if (i != (size_t)-1) {
		vfree(arr->u.aval.vals[i]);
		arr->u.aval.vals[i] = value;
	}
	else {
		if (!arr->u.aval.size || arr->u.aval.count >= arr->u.aval.size) { /* resize */
			arr->u.aval.size += 64;
			arr->u.aval.keys = (lexvalue **)realloc(arr->u.aval.keys, arr->u.aval.size * sizeof(lexvalue *));
			arr->u.aval.vals = (lexvalue **)realloc(arr->u.aval.vals, arr->u.aval.size * sizeof(lexvalue *));
		}
		i = arr->u.aval.count++;
		arr->u.aval.keys[i] = key;
		arr->u.aval.vals[i] = value;
	}
}

// }}}

char *_outbuffer = 0;
int _ob_length = 0;
int _ob_size = 0;

// {{{ void vbuf_clear()

void vbuf_clear() {
	_ob_length = 0;
	if (_ob_size > 4096) {
		FREE(_outbuffer);
		_outbuffer = 0;
		_ob_size = 0;
	}
}

// }}}

// {{{ int vbuf_write2(char *str)

int vbuf_write2(char *p) {
	unsigned int l;

//	printf(">>%s<<",p);
	l = strlen(p);
	if (_ob_length + l + 1 >= _ob_size) {
		_ob_size += l + (4096 - l&(4096-1));
		_outbuffer = (char *)realloc(_outbuffer, _ob_size);
	}
	memcpy(_outbuffer+_ob_length, p, l);
	_ob_length+=l;
	_outbuffer[_ob_length] = 0;
	return l;
}

// }}}

// {{{ int vbuf_write(lexvalue *val)

int vbuf_write(lexvalue *val) {
	char *p;
	char buf[4096];

	p = val2str(val);
	if (!p || !*p) return 0;

//	snprintf(buf,sizeof(buf)-1,"[%08lx]",val);
//	vbuf_write2(buf);
	return vbuf_write2(p);
}

// }}}

/* end of file */
