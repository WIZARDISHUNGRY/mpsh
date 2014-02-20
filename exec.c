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

exec.c:

	process execution

*/




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#ifdef Solaris
#include <wait.h>
#endif

#ifdef LINUX
#include <wait.h>
#endif

#ifdef BSD
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif



#define TRUE 1
#define FALSE 0


#include "mpsh.h"

char **build_env();
char **build_argv();


exec_command(command)
struct command *command;
{
	int prev_pipe[2], next_pipe[2];
	int job;
	int ret;
	int hist;

	prev_pipe[0] = prev_pipe[1] = -1;

	for(;;) {
		if(command->flags & FLAG_BATCH) {
			hist = add_history_entry(command);
			ret = call_batch(command);
#if defined BSD || defined LINUX
			if(command->job_handler) add_history_details(hist,0,NULL);
#else
			if(command->job_handler) add_history_details(hist,NULL);
#endif
		} else {
			if(command->flags & FLAG_PIPE)
				pipe(next_pipe);
			else
				next_pipe[0] = next_pipe[1] = -1;
			ret = call_exec(command,prev_pipe[0],next_pipe[1]);
		}

		if(ret == 0) return;

		prev_pipe[0] = next_pipe[0];
		prev_pipe[1] = next_pipe[1];

		if(command->pipeline) {
			if(command->smp_id) { /* Special job entry */
				job = add_job_smp(command);
			} else if(!(command->flags & FLAG_PIPE)) {
				if(command->flags & FLAG_COND) {
					job = add_job(command);
					if(wait_job(job) != 0) return;
				} else if(command->flags & FLAG_NEGCOND) {
					job = add_job(command);
					if(wait_job(job) == 0) return;
				} else if(command->flags & FLAG_SETOP_EQUALS) {
					job = add_job(command);
					if(wait_job(job) != 0) return;
				} else if(command->flags & FLAG_SETOP_SUBSET) {
					job = add_job(command);
					if(wait_job(job) != 0) return;
				} else if(command->flags & FLAG_SETOP_SUBSET_REV) {
					job = add_job(command);
					if(wait_job(job) != 0) return;
				} else {
					job = add_job(command);
					wait_job(job);
				}
			}

			command = command->pipeline;
		} else break;
	}
	/* Add job entry */
	if(ret) {
		if(ret == -2) {
			add_history_builtin(command);
		} else if(command->smp_id) {
			job = add_job_smp(command);
		} else {
			job = add_job(command);
			if((command->flags & FLAG_BACK) == 0x00)
				wait_job(job);
		}
	}
}



call_exec(command,pipe_in,pipe_out)
struct command *command;
int pipe_in, pipe_out;
{
	char **call_argv, **call_env;
	pid_t pid;
	char *path;
	pid_t pgrp;
	char *pt;
	int ret;


	if(!command->echo_text) {
		call_env = build_env(command);
		call_argv = build_argv(command);

		ret = try_exec_builtin(0,command);
		if(ret > 0) return(-2); /* Succeeded */

		path = find_path(command->words->word);
	}

	pid = fork();
	if(pid == 0) { /* child. */

		/* Process group stuff. */

		if((command->flags & FLAG_BACK) == 0x00) {
			signal(SIGTTOU,SIG_IGN);
			if(isatty(fileno(stdout))) {
				pgrp = getpid();
				setpgid(pgrp,pgrp);
				tcsetpgrp(control_term,pgrp);
			}
		} else {
			/*
			close(fileno(stdin));
			*/
#ifdef BSD
			setsid();
#else
			pgrp = getpid();
			setpgid(pgrp,pgrp);
#endif
		}

		/* Redirect IO */
		if(command->flags & FLAG_REDIRECT) {
			int fd;
			int flags;

			/* stdout */

			if(command->stdout_filename != NULL) {

				flags = O_WRONLY | O_CREAT;
				if(command->file_io_flags & IO_OUT_APPEND)
					flags |= O_APPEND;
				else
					flags |= O_TRUNC;

				fd = open(command->stdout_filename->word,flags,0666);
				if(fd != -1) {
					dup2(fd,fileno(stdout));
					close(fd);
				} else {
					report_error("Error redirecting stdout",
						command->stdout_filename->word,0,1);
					_exit(1);
				}
			}

			/* stderr */

			if(command->stderr_filename) {
				flags = O_WRONLY | O_CREAT;
				if(command->file_io_flags & IO_ERR_APPEND)
					flags |= O_APPEND;
				else
					flags |= O_TRUNC;
				fd = open(command->stderr_filename->word,flags,0666);
				if(fd != -1) {
					dup2(fd,fileno(stderr));
					close(fd);
				} else {
					report_error("Error redirecting stderr",
						command->stderr_filename->word,0,1);
					_exit(1);
				}
			}

		}

		/* Pipes */
		if(pipe_in != -1) {
			dup2(pipe_in,fileno(stdin));
			close(pipe_in);
		}

		if(pipe_out != -1) {
			if(command->flags & FLAG_PIPE) {
				if(command->pipe_io_flags & IO_OUT) {
					dup2(pipe_out,fileno(stdout));
				}
				if(command->pipe_io_flags & IO_ERR) {
					dup2(pipe_out,fileno(stderr));
				}
			} else {
				if(pipe_out != -1)
					dup2(pipe_out,fileno(stdout));
			}
			close(pipe_out);
		}

		/* Set operators */
		if(command->flags & FLAG_SETOP) {
			if(command->flags & FLAG_SETOP_UNION) {
				setop_union(command);
				_exit(0);
			}
			if(command->flags & FLAG_SETOP_INTER) {
				setop_intersection(command);
				_exit(0);
			}
			if(command->flags & FLAG_SETOP_DIFF) {
				setop_difference(command);
				_exit(0);
			}
			if(command->flags & FLAG_SETOP_SYMM) {
				setop_symmetric(command);
				_exit(0);
			}
			if(command->flags & FLAG_SETOP_EQUALS) {
				if(setop_equals(command) == 0)
					_exit(1);
				else
					_exit(0);
			}
			if(command->flags & FLAG_SETOP_SUBSET) {
				if(setop_subset(command) == 0)
					_exit(1);
				else
					_exit(0);
			}
			if(command->flags & FLAG_SETOP_SUBSET_REV) {
				if(setop_subset_rev(command) == 0)
					_exit(1);
				else
					_exit(0);
			}
		}

		if(command->flags & FLAG_GROUP) {
			if(command->flags & FLAG_BACK) {
#ifdef BSD
				/* we solved this earlier... */
#else
				setpgrp();
#endif
			}
			pt = command->words->word+1;
			pt[strlen(pt)-1] = '\0';
			if(command->flags & FLAG_BACK)
				parse_and_run(pt,NONINTERACTIVE);
			else
				parse_and_run(pt,INTERACTIVE);
			exit(0);
		}

		if(command->echo_text) {
			puts(command->echo_text);
			exit(0);
		}

		if(try_exec_builtin(1,command)) _exit(0);

		if(!path) {
			report_error("Command not found",command->words->word,0,0);
			_exit(1);
		}

		/* Finally, execute it. */
		if(command->flags & FLAG_NICE) nice(command->nice);
		execve(path,call_argv,call_env);
		report_error("Error executing command",path,0,1);
		_exit(1);
	} else { /* Parent process */
		if(pipe_in != -1) close(pipe_in);
		if(pipe_out != -1) close(pipe_out);
		/* parent. add job info on child */
		command->pid = pid;
	}

	if(!command->echo_text) {
		free(call_argv);
		free(call_env);
		if(path) free(path);
	}
	return(1);
}


call_batch(command)
struct command *command;
{
	int len;
	char *buff;
	char *handler;
	char str[12];
	char *args;
	char *tmp;


	sprintf(str,"handler-%c",command->job_handler);
	handler = get_env(str);
	if(!handler) {
		report_error("Unknown job handler",NULL,command->job_handler,0,0);
		return(0);
	}

	if(command->handler_args) {
		args = words_to_string(command->handler_args);
		len = strlen(handler) + strlen(args) + 2;
		tmp = strdup(handler);
		free(handler);
		handler = (char *) malloc(len);
		if(args[0]) 
			sprintf(handler,"%s %s",tmp,args);
		else
			strcpy(handler,tmp);
		free(tmp);
		free(args);
	}

	buff = words_to_string(command->words);
	send_to_command(command,buff,handler+1);
	free(handler);
	return(1);
}



char **build_env(command) 
struct command *command;
{
	struct word_list *w;
	int len, i;
	char **call_env;

	len=0;
	for(w=global_env->next; w->next; w=w->next) {
		if(public_env(w->word)) len++;
	}

	len += 2;

	call_env = (char **) malloc(sizeof(char *)*len);

	i=0;
	for(w=global_env->next; w->next; w=w->next) {
		if(public_env(w->word)) {
			call_env[i] = w->word;
			i++;
		} 
	}
	if(public_env(w->word)) {
		call_env[i] = w->word;
		i++;
	}

	call_env[i] = NULL;

	return(call_env);
}

char **build_argv(command)
struct command *command;
{
	struct word_list *w;
	int len, i;
	char **call_argv;


	len=0;
	for(w = command->words; w->next; w=w->next) len++;
	len += 2;
	call_argv = (char **) malloc(sizeof(char *)*len);
	i=0;
	for(w = command->words; w->next; w=w->next) {
		call_argv[i] = w->word;
		i++;
	}
	if(w->word[0]) {
		call_argv[i] = w->word;
		i++;
	}
	call_argv[i] = NULL;

	return(call_argv);
}



send_to_command(command,text,comm)
struct command *command;
char *text, *comm;
{
	int pid;
	int pipes[2];
	int wait_for_proc;
	int ret;

	if(comm[strlen(comm)-1] == '&')
		wait_for_proc = 0;
	else
		wait_for_proc = 1;

	pipe(pipes);
	pid = fork();
	if(pid == 0) { /* child. */
		dup2(pipes[0],fileno(stdin));
		close(pipes[1]);
		parse_and_run(comm,NONINTERACTIVE);
		_exit(1);
	} else { /* Parent process */
		close(pipes[0]);
		ret = write(pipes[1],text,strlen(text));
		close(pipes[1]);
		if(wait_for_proc)
			wait_for_process(pid);
	}
}

wait_for_process(pid)
int pid;
{
	siginfo_t infop;
	int st;

#ifdef BSD
	while(waitpid(pid,&st,WCONTINUED|WNOHANG|WUNTRACED) != pid)
		;
#else
	waitid(P_PID,pid,&infop,WEXITED);
#endif

	signal(SIGTTIN,SIG_IGN);
	signal(SIGTTOU,SIG_IGN);
}


struct word_list *words_from_command(text_command)
char *text_command;
{
	int pipes[2];
	int pid;
	char *output;
	int i;
	FILE *in;
	struct word_list *w, *words;

	output = (char *) malloc(1024);
	output[0] = '\0';

	pipe(pipes);
	pid = fork();
	if(pid == 0) { /* child. */
		dup2(pipes[1],fileno(stdout));
		close(pipes[1]);
		close(pipes[0]);
		/*
		devnull = open("/dev/null",O_WRONLY);
		dup2(devnull,fileno(stdin));
		*/
		parse_and_run(text_command,NONINTERACTIVE);
		_exit(1);
	} else { /* Parent process */
		close(pipes[1]);
		wait_for_process(pid);
		in = fdopen(pipes[0],"r");
		w = words = NULL;
		while(fgets(output,1024,in)) {
			i = strlen(output)-1;
			if(i >= 0 && output[i] == '\n')
				output[i] = '\0';
			if(w) {
				w->next = init_word_str(output);
				w = w->next;
			} else {
				words = init_word_str(output);
				w = words;
			}
		}
		fclose(in);
		free(output);
		return(words);
	}
}


char *read_from_command(text_command)
char *text_command;
{
	int pipes[2];
	int pid;
	char *output;
	int i;
	int ret;

	if(parse_depth++ > MAX_PARSE_DEPTH) {
		report_error("Excessive parse depth",NULL,0,0);
		return(NULL);
	}

	output = (char *) malloc(1024);
	output[0] = '\0';

	pipe(pipes);
	pid = fork();

	if(pid == 0) { /* child. */
		dup2(pipes[1],fileno(stdout));
		close(pipes[0]);
		close(pipes[1]);
		/*
		devnull = open("/dev/null",O_WRONLY);
		dup2(devnull,fileno(stdin));
		*/
		parse_and_run(text_command,NONINTERACTIVE);
		exit(1);
	} else { /* Parent process */
		close(pipes[1]);
		wait_for_process(pid);
		ret = read(pipes[0],output,1024);
		close(pipes[0]);
		output[ret] = '\0';
		i = strlen(output)-1;
		if(i >= 0 && output[i] == '\n')
			output[i] = '\0';
		return(output);
	}
}



