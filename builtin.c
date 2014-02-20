/*

Copyright (c) 2013, Dave Fischer 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions 
are met:

* Redistributions of source code must retain the above copyright notice,
	this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer 
	in the documentation and/or other materials provided with 
	the distribution.

* Neither the name of the The Center for Computational Aesthetics nor 
	the names of its contributors may be used to endorse or promote
	products derived from this software without specific prior 
	written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------

mpsh by Dave Fischer http://www.cca.org

builtin.c:

	mpsh builtin commands

*/



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef LINUX
#endif

#ifdef BSD
#endif


#include "mpsh.h"

struct builtins {
	char	*name;
	int		(*fn)();
} ;

int builtin_cd();
int builtin_setenv();
int builtin_jobs();
int builtin_hist();
int builtin_fg();
int builtin_wait();
int builtin_source();
int builtin_set();
int builtin_exit();
int builtin_alias();

struct builtins builtin_fns[] = {
	"fg",		builtin_fg,
	"wait",		builtin_wait,
	".",		builtin_source,
	"exit",		builtin_exit,
	"alias",	builtin_alias,
	"history",	builtin_hist,
	"cd",		builtin_cd,
	"setenv",	builtin_setenv,
	"jobs",		builtin_jobs,
	"{",		builtin_set,
	NULL,		NULL
} ;


try_exec_builtin(which,command)
int which;
struct command *command;
{
	int i;

	for(i=0; builtin_fns[i].name; i++) {
		if(strcmp(command->words->word,builtin_fns[i].name) == 0) {
			command->pid = -1;
			return((*(builtin_fns[i].fn))(command,which));
		}
	}
	return(0);
}

builtin_setenv(command,which) /* Display env variables */
struct command *command;
int which;
{
	char *arg;

	if(command->words->next)
		arg = command->words->next->word;
	else
		arg = NULL;

	if(!arg) {
		/*
		report_error("Missing setenv option",NULL,0,0);
		*/
		display_env(ENV_PUBLIC,0);
		return(1);
	}

	/*

	PARENT:

		setenv name=val
		setenv -d val

	CHILD:

		-s
		-sa
		-si
		-sh
		-q
		-qa
		-qi
		-qh
		-h

	*/

	if(which == PARENT) {
		if(arg[0] != '-') {
			set_env(command);
			return(1);
		}
		if(strcmp(arg,"-d") == 0) {
			if(command->words->next->next) 
				delete_env(command->words->next->next->word);
			else
				report_error("Missing setenv -d argument",NULL,0,0);
			return(1);
		} 
		return(0);
	} else {
		if(arg[0] != '-') {
			return(0);
		}

		if(strcmp(arg,"-s") == 0) {
			display_env(ENV_PUBLIC,0);
			return(1);
		}

		if(strcmp(arg,"-sa") == 0) {
			display_env(ENV_ALIAS,0);
			return(1);
		}

		if(strcmp(arg,"-si") == 0) {
			display_env(ENV_INTERNAL,0);
			return(1);
		}

		if(strcmp(arg,"-sh") == 0) {
			display_env(ENV_HANDLER,0);
			return(1);
		}

		if(strcmp(arg,"-q") == 0) {
			display_env(ENV_PUBLIC,1);
			return(1);
		}

		if(strcmp(arg,"-qa") == 0) {
			display_env(ENV_ALIAS,1);
			return(1);
		}

		if(strcmp(arg,"-qi") == 0) {
			display_env(ENV_INTERNAL,1);
			return(1);
		}

		if(strcmp(arg,"-qh") == 0) {
			display_env(ENV_HANDLER,1);
			return(1);
		}

		if(strcmp(arg,"-h") == 0) {
		puts("usage: setenv          # show environment variables");
		puts("       setenv name=val # set env variable [name] to [val]");
		puts("       setenv -s[aih]  # show var, alias, internal, handlers");
		puts("       setenv -q[aih]  # show quoted");
		puts("       setenv -d name  # delete [name] variable");
		return(1);
		}

#ifdef OBSOLETE
		/* This is changing to "clear" soon. */
		if(strcmp(arg,"-c") == 0) {
			clear_env();
			return(1);
		}
#endif

		report_error("Unknown setenv option",arg,0,0);
		return(1);
	}
}

builtin_jobs(command,which) /* Show current jobs */
struct command *command;
int which;
{
	char *arg;
	char *arg2;
	int job;

	arg = arg2 = NULL;

	if(command->words->next) {
		arg = command->words->next->word;
		if(command->words->next->next) 
			arg2 = command->words->next->next->word;
		else
			arg2 = NULL;
	} else
		arg = NULL;

	if(arg && strcmp(arg,"-d") == 0) {
		if(arg2) {
			delete_job_entry(arg2);
			return(1);
		} else {
			report_error("Missing jobs -d argument",NULL,0,0);
			return(1);
		}
	}

	if(which == PARENT) return(0);

	if(arg && strcmp(arg,"-h") == 0) {
		puts("usage: jobs            # Display current jobs");
		puts("       jobs %job       # Display full details for one job");
		puts("       jobs -s [%job]  # Display (specific) jobs");
		puts("       jobs -l [%job]  # Display (specific) jobs, long format");
		puts("       jobs fmt [%job] # Display (specific) jobs, [fmt] format");
		puts("       jobs -d %job    # Delete one job");
		return(1);
	}

	if(arg2)
		job = pid_to_job(arg2);
	else
		job = -1;

	if(arg && isdigit(arg[0])) {
		job = pid_to_job(arg);
		show_detailed_job(job);
		return(1);
	}

	show_jobs(arg,job);
	return(1);
}

builtin_hist(command,which) /* Display command history */
struct command *command;
int which;
{
	char *arg;
	char *arg2;

	/*

	CHILD PROCESS:

		history
		history -s
		history -l
		history [n]
		history [fmt]
		history -h

	PARENT PROCESS:

		history -c
	*/


	if(command->words->next) {
		arg = command->words->next->word;
		if(command->words->next->next)
			arg2 = command->words->next->next->word;
		else
			arg2 = NULL;
	} else {
		arg = NULL;
		arg2 = NULL;
	}

	if(which == PARENT) {
		if(arg && strcmp(arg,"-c") == 0) {
			clear_history();
			return(1);
		} else {
			return(0);
		}
	} else {
		if(arg != NULL) {
			if(strcmp(arg,"-h") == 0) {
				puts("usage: history        # show short format");
				puts("       history n      # show full details for one entry");
				puts("       history -s [n] # show short format");
				puts("       history -l [n] # show long format");
				puts("       history fmt    # show with [fmt] format");
				puts("       history -c     # clear all entries");
				return(1);
			}
		}
		show_history(arg,arg2);
		return(1);
	}
}

builtin_fg(command,which) /* Continue a stopped job */
struct command *command;
int which;
{
	char *arg;

	if(command->words->next)
		arg = command->words->next->word;
	else
		arg = NULL;

	if(arg && strcmp(arg,"-h") == 0) {
		if(which == PARENT) return(0);
		else {
			puts("usage: fg      # Resume default stopped job");
			puts("       fg %-   # Resume previous stopped job");
			puts("       fg %job # Resume specified job");
			return(1);
		}
	}

	fg_job(arg);
	return(1);
}

builtin_set(command,which) /* Display a set. */
struct command *command;
int which;
{
	if(which == PARENT) return(0);

	show_set(command->words->next);
	return(1);
}

builtin_source(command,which) /* Execute a script in the parent shell process */
struct command *command;
int which;
{
	int i;
	char *script;
	struct word_list *w;
	char buff[128];

	if(!command->words->next) {
		report_error("Missing source filename",NULL,0,0);
		return(1);
	}

	script = command->words->next->word;

	if(strcmp(script,"-h") == 0) {
		if(which == PARENT) return(0);
		else {
puts("usage: . script [args] # Run a script without forking a child process");
			return(1);
		}
	}

	i = 0;

	for(w=command->words->next->next; w; w=w->next) {
		i++;
		sprintf(buff,"%d=%s",i,w->word);
		set_env_str(buff);
	}

	sprintf(buff,"#=%d",i);
	set_env_str(buff);

	run_script(script,0);
	return(1);
}

builtin_exit(command,which)
struct command *command;
int which;
{
	char *arg;
	int value;

	if(command->words->next)
		arg = command->words->next->word;
	else
		arg = NULL;

	if(arg && strcmp(arg,"-h") == 0) {
		if(which == PARENT) return(0);
		else {
			puts("usage: exit [value] # Exit with optional exit value");
			return(1);
		}
	}

	if(arg)
		value = atoi(arg);
	else
		value = 0;
	
	exit(value);
}


