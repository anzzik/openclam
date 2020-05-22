#include <stdio.h>
#include <stdlib.h>

#include "shell.h"

int main(void)
{
	int r;

	r = shell_init();
	if (r)
	{
		fprintf(stderr, "shell_init failed\n");
		return 1;
	}

	r = shell_mainloop();
	shell_free();

	return r;
}

