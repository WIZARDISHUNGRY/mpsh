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

main.c:

	main()

	Kick off initialization, and the top level of the
	prompt/input/parse/exec loop.

*/




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef IRIX
#endif

#ifdef LINUX
#endif

#ifdef BSD
#endif




#ifdef READLINE
#include <readline/readline.h>
#endif



#include "mpsh.h"
#include "state.h"

struct word_list *global_env;
struct word_list *search_path;

int control_term;



main(argc,argv,env)
int argc;
char *argv[];
char *env[];
{
#ifdef READLINE
	char *line;
#else
	char line[1024];
#endif
	char *pr;


	init_all(argc,argv,env);

	setpgid(getpid(),getpid());
	signal(SIGTTOU,SIG_IGN);
	tcsetpgrp(control_term,getpgrp());


#ifdef READLINE
	for(;;) {
		parse_depth = 0;
		if(line = readline(pr=get_prompt())) {
			parse_and_run(line,INTERACTIVE);
		} else {
			/* EOF */
			if(number_of_jobs() > 0 ) {
				puts("");
				show_jobs(NULL,-1);
			} else {
				puts("");
				if(strcmp(get_env("mpsh-eof-exit"),"1") == 0) 
					exit(0);
			}
		}
		free(pr);
	}

#else
	for(;;) {
		parse_depth = 0;
		pr = get_prompt();
		printf("%s",pr);
		free(pr);
		if(gets(line)) {
			parse_and_run(line,INTERACTIVE);
		} else {
			/* EOF */
			if(number_of_jobs() > 0 ) {
				puts("");
				show_jobs(NULL,-1);
			} else {
				puts("");
				if(strcmp(get_env("mpsh-eof-exit"),"1") == 0) 
					exit(0);
			}
		}
	}
#endif

}

parse_and_run(text_command,interactive)
char *text_command;
int interactive;
{
	char *pt;
	struct command *command;
	struct command *initial_command;

	if(parse_depth++ > MAX_PARSE_DEPTH) {
		report_error("Excessive parse depth",NULL,0,0);
		return(-1);
	}

	/* Init parse tree structs */
	command = initial_command = init_command();
	command->text = (char *) malloc(1024);
	command->text[0] = '\0';
	command->state = STATE_SPACE;

	pt = text_command;

	while(command->state != STATE_DONE) {
		if(*pt) {
			add_command_text_letter(initial_command,*pt);
			command = parse_char(command,*pt);
			if(!command) return(0);
			pt++;
		} else {
			command = parse_char(command,'\n');
			if(!command) return(0);
		}
	}

	if(initial_command->words != NULL) {
		/* MORE PARSING */
		shift_set_operators(initial_command);
		expand_globbing(initial_command);
		expand_job_num(initial_command);
		if(!expand_env(initial_command)) goto skip;

		/* This goes in the last command struct in the pipeline,
		   rather than the first, because of the job entry mechanism.
		*/
		command->expansion = get_command_expansion(initial_command);

		if(!expand_mp(initial_command)) goto skip;

		/* DEBUG */
		/*
		display_command(initial_command);
		*/

		if((command->flags & FLAG_DISPLAY_TEXT) && interactive)
			puts(initial_command->text);

		/* Save this info for history, before exec command */
		command->dir = dup_cwd();

		/* EXECUTE COMMAND */
		exec_command(initial_command);
	}

	check_for_job_exit();

	skip:

	free_command(initial_command);

	return;
}


