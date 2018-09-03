/*
 * parse.c - feeble command parsing for the Feeble SHell.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "parse.h"
#include "error.h"


#define MAXARGV 1000

enum token {
    identifier, directin, directout, doubledirectout,
    ampersand,
    /* everything >= verticalbar ends an individual "struct pipeline" */
    verticalbar, doublepipe,
    eol
};
static enum token gettoken(char **s, char **argp);
static char *ptok(enum token tok);


struct parsed_line *parse(char *s)
{
    struct parsed_line *retval;  /* remains freeparse()able at all times */
    struct pipeline **plp;  /* where to append for '|' and '|||' */
    char *argv[MAXARGV];
    enum token tok;
    int argc = 0;
    int isdouble = 0;

    retval = emalloc(sizeof(struct parsed_line));
    retval->inputfile = retval->outputfile = NULL;
    retval->output_is_double = 0;
    retval->isbg = 0;
    retval->pl = NULL;
    plp = &(retval->pl);

    do {
        if (argc >= MAXARGV)
            fatal("argv limit exceeded");
        while ((tok = gettoken(&s, &argv[argc])) < verticalbar) {
	    switch ((int)tok) {  /* cast prevents stupid warning message about
				  * not handling all enum token values */
	    case identifier:
		argc++;  /* it's already in argv[argc];
		          * increment to represent a save */
		break;
	    case directin:
		if (retval->inputfile) {
		    fprintf(stderr,
			    "syntax error: multiple input redirections\n");
		    freeparse(retval);
		    return(NULL);
		}
		if (gettoken(&s, &retval->inputfile) != identifier) {
		    fprintf(stderr, "syntax error in input redirection\n");
		    freeparse(retval);
		    return(NULL);
		}
		break;
	    case doubledirectout:
		retval->output_is_double = 1;
		/* fall through */
	    case directout:
		if (retval->outputfile) {
		    fprintf(stderr,
			    "syntax error: multiple output redirections\n");
		    freeparse(retval);
		    return(NULL);
		}
		if (gettoken(&s, &retval->outputfile) != identifier) {
		    fprintf(stderr, "syntax error in output redirection\n");
		    freeparse(retval);
		    return(NULL);
		}
		break;
	    case ampersand:
		retval->isbg = 1;
		break;
	    }
	}

	/* cons up just-parsed pipeline component */
	if (argc) {
	    *plp = emalloc(sizeof(struct pipeline));
	    (*plp)->next = NULL;
	    (*plp)->argv = eargvsave(argv, argc);
	    (*plp)->isdouble = isdouble;
	    plp = &((*plp)->next);
	    isdouble = 0;
	    argc = 0;
	} else if (tok != eol) {
	    fprintf(stderr, "syntax error: null command before `%s'\n",
		    ptok(tok));
	    freeparse(retval);
	    return(NULL);
	}

	/* is this a funny kind of pipe (to the right)? */
	if (tok == doublepipe)
	    isdouble = 1;

    } while (tok != eol);
    return(retval);
}


/* (*s) is advanced as we scan; *argp is set iff retval == identifier */
static enum token gettoken(char **s, char **argp)
{
    char *p;

    while (**s && isascii(**s) && isspace(**s))
        (*s)++;
    switch (**s) {
    case '\0':
        return(eol);
    case '<':
        (*s)++;
        return(directin);
    case '>':
        (*s)++;
	if (**s == '&') {
	    (*s)++;
	    return(doubledirectout);
	}
        return(directout);
    case '|':
        (*s)++;
	if (**s == '&') {
	    (*s)++;
	    return(doublepipe);
	}
        return(verticalbar);
    case '&':
	(*s)++;
	return(ampersand);
    /* else identifier */
    }

    /* it's an identifier */
    /* find the beginning and end of the identifier */
    p = *s;
    while (**s && isascii(**s) && !isspace(**s) && !strchr("<>;&|", **s))
        (*s)++;
    *argp = estrsavelen(p, *s - p);
    return(identifier);
}


static char *ptok(enum token tok)
{
    switch (tok) {
    case directin:
	return("<");
    case directout:
	return(">");
    case verticalbar:
	return("|");
    case ampersand:
	return("&");
    case doubledirectout:
	return(">&");
    case doublepipe:
	return("|&");
    case eol:
	return("end of line");
    default:
	return(NULL);
    }
}


static void freepipeline(struct pipeline *pl)
{
    if (pl) {
	char **p;
        for (p = pl->argv; *p; p++)
            free(*p);
        free(pl->argv);
        freepipeline(pl->next);
        free(pl);
    }
}


void freeparse(struct parsed_line *p)
{
    if (p) {
	if (p->inputfile)
	    free(p->inputfile);
	if (p->outputfile)
	    free(p->outputfile);
	freepipeline(p->pl);
	free(p);
    }
}
