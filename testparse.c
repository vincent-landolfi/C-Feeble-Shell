/*
 * This is a test module for the parser for fsh (Feeble SHell).
 * This is not part of fsh itself.
 * Compile with parse.c and error.c (only).
 * Type input commands to it and see the parsed data structure.
 * (Arguably this file should not be printing the '$ ' prompt, but it seems
 * easier to use if it does.)
 */

#include <stdio.h>
#include "parse.h"
#include "error.h"

int main()
{
    char buf[1000];
    struct parsed_line *p;
    extern void show(struct parsed_line *p);

    while (printf("$ "), fgets(buf, sizeof buf, stdin)) {
        if ((p = parse(buf))) {
            show(p);
            freeparse(p);
        }
    }

    return(0);
}


void show(struct parsed_line *p)
{
    struct pipeline *pl;
    char **argp;
    int something;

    something = 0;

    if (p->inputfile) {
	printf("redirect input from \"%s\"\n ", p->inputfile);
	something = 1;
    }

    for (pl = p->pl; pl; pl = pl->next) {
	if (pl != p->pl)
	    printf("    %spiped to: ", pl->isdouble ? "double-" : "");
	printf("argv is");
	for (argp = pl->argv; *argp; argp++)
	    printf(" \"%s\"", *argp);
	printf("\n");
	something = 1;
    }

    if (p->outputfile) {
	printf("  redirect %s to \"%s\"\n",
		p->output_is_double ?  "stdout+stderr" : "output",
		p->outputfile);
	something = 1;
    }
    if (p->isbg) {
	printf("  [backgrounded]\n");
	something = 1;
    }

    if (!something)
	printf("\n");

}
