/*
 * YATE template engine
 *
 * $Id: main_cli.c,v 1.1 2003/12/21 01:09:42 mclap Exp $
 */

#include <stdio.h>
#include <string.h>
#include "yate.h"

int main (int ac, char **av) {
	yate *y;
	if (ac !=2) {
		printf("usage: %s template\n", av[0]);
		return 0;
	}

	y = yate_init();
	yate_exec(y, av[1]);
	printf("buffer: %08lx\nsize: %d/%d\n", y->out.buf, y->out.len, y->out.size);
	printf("=======================\n");
	printf("%s", y->out.buf);
	yate_free(y);
}
