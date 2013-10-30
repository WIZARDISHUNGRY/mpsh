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
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef LINUX
#include <stdlib.h>
#include <string.h>
#endif

#ifdef BSD
#include <stdlib.h>
#include <string.h>
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
int builtin_clear();
int builtin_show();

struct builtins builtin_fns_parent[] = {
	"fg",		builtin_fg,
	"wait",		builtin_wait,
	".",		builtin_source,
	/* "mpsh-clear", builtin_clear, */
	"exit",		builtin_exit,
	/* Either parent or child: */
	"history",	builtin_hist,
	"cd",		builtin_cd,
	"setenv",	builtin_setenv,
	NULL,		NULL
} ;

struct builtins builtin_fns_child[] = {
	"jobs",		builtin_jobs,
	/* "mpsh-show", builtin_show, */
	"{",		builtin_set,
	/* Either parent or child: */
	"history",	builtin_hist,
	"cd",		builtin_cd,
	"setenv",	builtin_setenv,
	NULL,		NULL
} ;




is_child_builtin(text)
char *text;
{
	struct builtins *builtin;
	int i;

	builtin = builtin_fns_child;

	for(i=0; builtin[i].name; i++) {
		if(strcmp(text,builtin[i].name) == 0) {
			return(1);
		}
	}
	return(0);
}

try_exec_builtin(which,command)
int which;
struct command *command;
{
	struct builtins *builtin;
	int i;

	if(which == 0)
		builtin = builtin_fns_parent;
	else
		builtin = builtin_fns_child;

	for(i=0; builtin[i].name; i++) {
		if(strcmp(command->words->word,builtin[i].name) == 0) {
			command->pid = -1;
			return((*(builtin[i].fn))(command,which));
			/*
			return(1);
			*/
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
		report_error("Missing setenv option",NULL,0,0);
		return(1);
	}

	/*

	PARENT:

		setenv name=val
		setenv -d val
	
	CHILD:

		setenv -s
		setenv -i
		setenv -c

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
			show_env_public();
			return(1);
		}
		if(strcmp(arg,"-c") == 0) {
			show_env_private();
			return(1);
		}
		if(strcmp(arg,"-i") == 0) {
			show_env_mpsh();
			return(1);
		}
		return(0);
	}
}

builtin_jobs(command,which) /* Show current jobs */
struct command *command;
int which;
{
	show_jobs();
	return(1);
}

builtin_hist(command,which) /* Display command history */
struct command *command;
int which;
{
	struct word_list *arg;

	/*

	CHILD PROCESS:

		history
		history -s
		history -l
		history [n]

	PARENT PROCESS:

		history -c
	*/

	arg = command->words->next;

	if(which == PARENT) {
		if(arg && strcmp(arg->word,"-c") == 0) {
			init_history();
			return(1);
		} else {
			return(0);
		}
	} else {
		if(arg && strcmp(arg->word,"-c") == 0) {
			return(0);
		} else {
			if(arg)
				show_history(arg->word);
			else
				show_history(NULL);
			return(1);
		}
	}
}

builtin_fg(command,which) /* Continue a stopped job */
struct command *command;
int which;
{
	if(command->words->next)
		fg_job(command->words->next->word);
	else
		fg_job(NULL);
	return(1);
}

builtin_set(command,which) /* Display a set. */
struct command *command;
int which;
{
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
		return(0);
	}

	script = command->words->next->word;

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
	if(command->words->next) 
		exit(atoi(command->words->next->word));
	else
		exit(0);
}

builtin_clear(command,which)
struct command *command;
int which;
{
	char *pt;

	if(command->words->next) {
		pt = command->words->next->word;

		if(strcmp(pt,"-d") == 0) {
			init_cdhistory();
			return;
		}

		if(strcmp(pt,"-h") == 0) {
			init_history();
			return;
		}

		if(strcmp(pt,"-e") == 0 && command->words->next->next) {
			delete_env(command->words->next->next->word);
			return;
		}

	}

	puts("usage: mpsh-clear [-d] [-h] [-e var]");
}

builtin_show(command,which)
struct command *command;
int which;
{
	show_env(command);
}

