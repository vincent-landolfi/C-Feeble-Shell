/*
 * fsh.c - the Feeble SHell.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <getopt.h>
#include "fsh.h"
#include "parse.h"
#include "error.h"

int showprompt = 1;
int laststatus = 0;  /* set by anything which runs a command */


int main(int argc, char** argv)
{
	int optioni = 0;
	int optionv = 0;
	int optionc = 0;
	FILE* fp;
    char buf[1000];
    struct parsed_line *p;
    extern void execute(struct parsed_line *p);
    int option;
	while ((option = getopt(argc, argv, "c:iv")) != -1) {
		switch (option) {
			case 'i' :
				optioni = 1;
				break;
			case 'v' :
				optionv = 1;
				break;
			case 'c' :
				optionc = 1;
				strcpy(buf,optarg);
				break;
		}
	}
	if (argv[optind] != NULL && optionc) {
		fprintf(stderr,"File given with option -c \n");
		return(1);
	}
	if (argv[optind] != NULL) {
		if((fp = fopen(argv[optind],"r")) == NULL) {
			perror(argv[optind]);
			return(1);
		}
	}
    while (1) {
	if (showprompt) {
		if (((isatty(1) || optioni) && !optionc && fp == NULL) || (fp != NULL && optioni)){
	    	printf("$ ");
	    }
	}
	if ((strcmp(buf,"") == 0 && fp == NULL)) {
		if (fgets(buf, sizeof buf, stdin) == NULL)
	    	break;
	} else if (fp != NULL && strcmp(buf,"") == 0 && fgets(buf,sizeof buf,fp)) {
		// does all the work in the if statement
	} else if (!optionc){
		break;
	}
	if ((p = parse(buf))) {
		if (optionv) {
			printf("%s",buf);
		}
	    execute(p);
	    freeparse(p);
	    if (optionc) {
	    	break;
	    }
	    strcpy(buf,"");
        }
    }

    return(laststatus);
}


void execute(struct parsed_line *p)
{
    int status;
    extern void execute_one_subcommand(struct parsed_line *p);

    fflush(stdout);
    switch (fork()) {
    case -1:
	perror("fork");
	laststatus = 127;
	break;
    case 0:
	/* child */
	execute_one_subcommand(p);
	break;
    default:
	/* parent */
	wait(&status);
	laststatus = status >> 8;
    }
}

/*
 * execute_one_subcommand():
 * Do file redirections if applicable, then [you can fill this in...]
 * Does not return, so you want to fork() before calling me.
 */
void execute_one_subcommand(struct parsed_line *p)
{
	char *paths[] = {"/bin", "/usr/bin","."};
	char dir[128];
	int index = 0;
	int flag = 1;
	int pid;
	int mypipefd[2];
	struct stat statbuf;
	extern char** environ;
    if (p->inputfile) {
	close(0);
	if (open(p->inputfile, O_RDONLY, 0) < 0) {
	    perror(p->inputfile);
	    laststatus = 1;
	}
    }
    if (p->outputfile) {
	close(1);
	if (open(p->outputfile, O_WRONLY|O_CREAT|O_TRUNC, 0666) < 0) {
	    perror(p->outputfile);
	    laststatus = 1;
	}
	if (p->output_is_double){
    	dup2(1,2);
    }
    }
    if (strchr(p->pl->argv[0],'/') != NULL) {
    	strcpy(dir,p->pl->argv[0]);
    	flag = 0;
    	if (stat(dir, &statbuf)) {
    		flag = 1;
    	}
    } else {
    	while(flag && index<3) {
    		strcpy(dir, (efilenamecons(paths[index],p->pl->argv[0])));
    		if (stat(dir, &statbuf)) {
    			index ++;
    		} else {
    			flag = 0;
    		}
    	}
	}
    if (p->pl) {
		if (p->pl->next != NULL){
			fflush(stdout);
			switch((pid = fork())) {
			case -1:
				perror("fork");
				break;
			case 0:
				/*child*/
				if (pipe(mypipefd)) {
					perror("pipe");
					laststatus = 127;
				}
				switch (fork()){
				case -1:
					perror("fork");
					laststatus = 127;
				case 0:
					if(p->pl->next->isdouble) {
						dup2(mypipefd[1],2);
					}
					close(mypipefd[0]);
					dup2(mypipefd[1],1);
					close(mypipefd[1]);
					execv(dir,p->pl->argv);
					fprintf(stderr, "%s: Command not found\n",p->pl->argv[0]);
					laststatus = 126;
					exit(laststatus);
				default:
					close(mypipefd[1]);
					dup2(mypipefd[0],0);
					close(mypipefd[0]);
					flag = 1;
					index = 0;
					while(flag && index<3) {
    					strcpy(dir, (efilenamecons(paths[index],p->pl->next->argv[0])));
    				if (stat(dir, &statbuf)) {
    					index ++;
    				} else {
    					flag = 0;
    				}
    				}
					execv(dir,p->pl->next->argv);
					fprintf(stderr, "%s: Command not found\n",p->pl->next->argv[0]);
					laststatus = 125;
				}
				break;
			default:
				pid = wait(&laststatus);
			}
		} else {
			execve(dir,p->pl->argv,environ);
			perror("exec error");
			laststatus = 1;
		}
	} else {
		fprintf(stderr, "%s: Command not found\n",p->pl->argv[0]);
	}
    exit(laststatus);
}
