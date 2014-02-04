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

jobs.c:

	job handling functions

*/




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#ifdef Solaris
#include <wait.h>
#endif

#ifdef IRIX
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




#include "mpsh.h"


struct job {
	char *text;
	pid_t pid;
	pid_t *pids;
	int history;
	int running;
	int smp_id;
	int smp;
	int last_fg;
	int group;
} ;


#define JOB_ENTRIES 1024
struct job jobs[JOB_ENTRIES];

int current_job_pid;
int default_bg_job;


init_jobs() {
	int i;

	for(i=0; i<JOB_ENTRIES; i++) {
		jobs[i].pid = 0;
		jobs[i].pids = NULL;
		jobs[i].smp_id = 0;
		jobs[i].last_fg = 0;
		jobs[i].group = 0;
	}

	current_job_pid = -1;
	default_bg_job = -1;
}

add_job(command)
struct command *command;
{
	int j;

	if(command->flags & FLAG_BATCH) return(-1);

	for(j=0; j<JOB_ENTRIES; j++) {
		if(jobs[j].pid == 0) goto found_slot;
	}

	report_error("Too many jobs",NULL,0,0);
	return(-1);

	found_slot:

	jobs[j].text = strdup(command->text);
	jobs[j].pid = command->pid;
	jobs[j].history = add_history_entry(command);
	jobs[j].running = 1;
	jobs[j].last_fg = 0;
	jobs[j].smp = command->smp_num;


	if(command->flags & FLAG_NOTERM)
		jobs[j].group = 1;
	else
		jobs[j].group = 0;

	return(j);
}


add_job_smp(command)
struct command *command;
{
	int j;
	int i;

	for(j=0; j<JOB_ENTRIES; j++) {
		if(jobs[j].smp_id == command->smp_id)
			goto found;
	}

	j = add_job(command);

	if(j == -1) return(-1);

	jobs[j].smp_id = command->smp_id;
	jobs[j].pids = (pid_t *) malloc(sizeof(pid_t)*(command->smp_num+1));
	jobs[j].pids[0] = command->pid;
	jobs[j].pids[1] = 0;
	jobs[j].pid = -1;

	return(j);

	found:

	for(i=0; jobs[j].pids[i]; i++) ;

	jobs[j].pids[i++] = command->pid;
	jobs[j].pids[i] = 0;

	return(j);
}

show_jobs()
{
	int j;
	int i;
	char bg_status;
	int second_bg_job;

	second_bg_job = get_second_bg_job();

	for(j=0; j<JOB_ENTRIES; j++) {
		if(j == default_bg_job) bg_status = '+';
		else if(j == second_bg_job) bg_status = '-';
		else bg_status = ' ';

		if(jobs[j].pids) {
			for(i=0; jobs[j].pids[i]; i++) ;
			printf("%2d %c%c %s (%d/%d)\n",j,
				jobs[j].running ? 'R' : 'S', bg_status,
				jobs[j].text,i,jobs[j].smp);
		} else if(jobs[j].pid)
			printf("%2d %c%c %s\n",j,
				jobs[j].running ? 'R' : 'S', bg_status,
				jobs[j].text);
	}

}


delete_job(j)
int j;
{
	jobs[j].pid = 0;
	jobs[j].smp_id = 0;
	if(jobs[j].pids) {
		free(jobs[j].pids);
		jobs[j].pids = NULL;
	}
	if(jobs[j].text) {
		free(jobs[j].text);
		jobs[j].text = NULL;
	}
	return;
}

#ifdef BSD

wait_job(job) /* BSD VERSION */
int job;
{
	int p;
	struct rusage ruse;
	int ret;
	int st;
	int wp;

	if(job == -1) return(0);

	ret = 1;

	p = jobs[job].pid;
	current_job_pid = p;

	wp = wait4(p,&st,WUNTRACED,&ruse);

	if(wp != p) {
		return(-1);
	}

	signal(SIGTTIN,SIG_IGN);
	signal(SIGTTOU,SIG_IGN);
	if(!jobs[job].group)
		tcsetpgrp(control_term,getpgrp());

	current_job_pid = -1;

	if(WIFSIGNALED(st)) {
		puts("Killed");
		ret = -1;
	} 

	if(WIFSTOPPED(st)) {
		puts("Stopped");
		printf("Job: %s\n",jobs[job].text);
		jobs[job].running = 0;
		change_history_status(jobs[job].history,"Stopped");
		default_bg_job = job;
		inc_last_fg();
		jobs[job].last_fg = 1;
	} else {
		add_history_details(jobs[job].history,st,&ruse);
		delete_job(job);
		find_bg_job();
		ret = WEXITSTATUS(st);
	}

	return(ret);
}

check_for_job_exit() { /* BSD VERSION */
	int pid;
	int job;
	int i, k;
	struct rusage ruse;
	int st;


	while((pid = wait4(WAIT_ANY,&st,WUNTRACED|WNOHANG,&ruse)) > 0) {
		signal(SIGTTIN,SIG_IGN);
		signal(SIGTTOU,SIG_IGN);
		if(pid == 0) return(0);
		for(job=0; job<JOB_ENTRIES; job++) {
			if(jobs[job].pids != 0) {
				for(i=0; jobs[job].pids[i]; i++) {
					if(jobs[job].pids[i] == pid) {
						for(k=i; jobs[job].pids[k]; k++) 
							jobs[job].pids[k] = jobs[job].pids[k+1];
						if(jobs[job].pids[0] == 0) {
							fprintf(stderr,"Job %s %s.\n",jobs[job].text,
								WIFSIGNALED(st) ?
								"killed" : "exited");
							add_history_details(jobs[job].history,st,&ruse);
							delete_job(job);
						} else
							add_history_time(jobs[job].history,st,&ruse);
						goto end;
					}
				}
			} else if(jobs[job].pid == pid) {
				if(!WIFSTOPPED(st)) {
					fprintf(stderr,"Job %s %s.\n",jobs[job].text,
						WIFSIGNALED(st) ? "killed" : "exited");
					add_history_details(jobs[job].history,st,&ruse);
					delete_job(job);
					goto end;
				}
			}
		}
		end: ;
	}
}

check_for_job_wake(dp) /* BSD VERSION */
int dp;
{ 
	int pid;
	int job;
	struct rusage ruse;
	int st;

	while((pid = wait4(dp,&st,WCONTINUED,&ruse)) != 0) {
		if(pid != 0) {
			for(job=0; job<JOB_ENTRIES; job++) {
				if(jobs[job].pid == pid) {
					jobs[job].running = 1;
					change_history_status(jobs[job].history,"Running");
					wait_job(job);
					return;
				}
			}
		} else return;
	}
}

#else

wait_job(job) /* NON-BSD VERSION */
int job;
{
	int p;
	siginfo_t infop;
	int ret;

	if(job == -1) return(0);

	ret = 1;

	p = jobs[job].pid;
	current_job_pid = p;

	infop.si_utime = 0L;
	infop.si_stime = 0L;

	waitid(P_PID,p,&infop,WEXITED|WSTOPPED);

	signal(SIGTTIN,SIG_IGN);
	signal(SIGTTOU,SIG_IGN);
	if(!jobs[job].group)
		tcsetpgrp(control_term,getpgrp());

	current_job_pid = -1;

	if(infop.si_code == CLD_KILLED || infop.si_code == CLD_DUMPED) {
		puts("Killed");
		ret = -1;
	}

	if(infop.si_code == CLD_STOPPED) {
		puts("Stopped");
		printf("Job: %s\n",jobs[job].text);
		jobs[job].running = 0;
		change_history_status(jobs[job].history,"Stopped");
		default_bg_job = job;
		inc_last_fg();
		jobs[job].last_fg = 1;
	} else {
		add_history_details(jobs[job].history,&infop);
		delete_job(job);
		find_bg_job();
		ret = infop.si_status;
	}

	return(ret);
}

check_for_job_exit() { /* NON-BSD VERSION */
	int pid;
	siginfo_t infop;
	int job;
	int i, k;


	infop.si_utime = 0L;
	infop.si_stime = 0L;

	while(waitid(P_ALL,0,&infop,WEXITED|WNOHANG) == 0) {
		signal(SIGTTIN,SIG_IGN);
		signal(SIGTTOU,SIG_IGN);
		pid = infop.si_pid;
		if(pid == 0) return(0);
		for(job=0; job<JOB_ENTRIES; job++) {
			if(jobs[job].pids != 0) {
				for(i=0; jobs[job].pids[i]; i++) {
					if(jobs[job].pids[i] == pid) {
						for(k=i; jobs[job].pids[k]; k++) 
							jobs[job].pids[k] = jobs[job].pids[k+1];
						if(jobs[job].pids[0] == 0) {
							fprintf(stderr,"Job %s %s.\n",jobs[job].text,
								infop.si_code == CLD_KILLED ||
								infop.si_code == CLD_DUMPED ?
								"killed" : "exited");
							add_history_details(jobs[job].history,&infop);
							delete_job(job);
						} else
							add_history_time(jobs[job].history,&infop);
						goto end;
					}
				}
			} else if(jobs[job].pid == pid) {
				fprintf(stderr,"Job %s %s.\n",jobs[job].text,
					infop.si_code == CLD_KILLED ||
					infop.si_code == CLD_DUMPED ? "killed" : "exited");
				add_history_details(jobs[job].history,&infop);
				delete_job(job);
				goto end;
			}
		}
		end: ;
	}
}

check_for_job_wake(dp) /* NON-BSD VERSION */
int dp;
{
	int pid;
	siginfo_t infop;
	int job;

	while(waitid(P_PID,dp,&infop,WCONTINUED) == 0) {
		pid = infop.si_pid;
		if(pid == dp) {
			for(job=0; job<JOB_ENTRIES; job++) {
				if(jobs[job].pid == pid) {
					jobs[job].running = 1;
					change_history_status(jobs[job].history,"Running");
					wait_job(job);
					return;
				}
			}
		} else return;
	}
}

#endif

fg_job(arg)
char *arg;
{
	pid_t pid;

	if(arg) {
		pid = (pid_t) atoi(arg);
	} else {
		if(default_bg_job == -1) find_bg_job();
		if(jobs[default_bg_job].pid == 0) find_bg_job();
		pid = jobs[default_bg_job].pid;
	}

	if(pid > 0) {
		signal(SIGTTOU,SIG_IGN);
		tcsetpgrp(control_term,getpgid(pid));
		kill(pid * -1,SIGCONT);
		check_for_job_wake(pid);
	} else {
		report_error("Unknown job",arg,0,0);
		return(0);
	}
}

expand_job_num(curr)
struct command *curr;
{
	struct word_list *w;
	struct word_list *later_words, *prev;
	int job;
	char buff[32];
	int i;

	while(curr) {
		w = curr->words->next;
		while(w) {
			if(w->word && w->word[0] == '%') {
				/* Expand job number. */
				if(w->word[1] == '%') { /* default job */
					job = default_bg_job;
				} else if(w->word[1] == '-') { /* next default job */
					job = get_second_bg_job();
				} else if(isdigit(w->word[1])) {
					job = atoi(w->word+1);
				} else {
					job = find_job_by_name(w->word+1);
				}
				if(job != -1 && jobs[job].pid != 0) {
					if(jobs[job].pids) {
						later_words = w->next;
						for(i=0; jobs[job].pids[i]; i++) {
							sprintf(buff,"%d",jobs[job].pids[i]);
							string_to_word(buff,&w);
							prev = w;
							w->next = init_word();
							w = w->next;
						}
						prev->next = later_words;
					} else {
						sprintf(buff,"%d",jobs[job].pid);
						string_to_word(buff,&w);
					}
				}

			}
			w = w->next;
		}
		curr = curr->pipeline;
	}
}

find_job_by_name(str)
char *str;
{
	int len;
	int len2;
	char *cmp;
	int i;
	int job;

	len = strlen(str);
	for(job=0; job<JOB_ENTRIES; job++) {
		if(jobs[job].pid &&
		strncmp(str,jobs[job].text,len) == 0) return(job);
	}

	for(job=0; job<JOB_ENTRIES; job++) {
		if(jobs[job].pid != 0) {
			cmp = jobs[job].text;
			len2 = strlen(cmp)-1;
			for(i=1; i<len2; i++)
				if(strncmp(str,cmp+i,len) == 0) return(job);
		}
	}
	return(-1);
}

builtin_wait(command,which)
struct command *command;
int which;
{
	int j;

	if(command->words) {
		j = atoi(command->words->word);
		while(jobs[j].pid != 0)
			check_for_job_exit();
	} else {
		for(j=0; j<JOB_ENTRIES; j++) {
			while(jobs[j].pid != 0)
				check_for_job_exit();
		}
	}
}

inc_last_fg() {
	int j;

	for(j=0; j<JOB_ENTRIES; j++) {
		if(jobs[j].pid != 0) {
			if(jobs[j].last_fg > 0)
				jobs[j].last_fg++;
		}
	}
}

find_bg_job() {
	int j;
	int found;
	int lowest;

	lowest = JOB_ENTRIES;
	found = -1;

	for(j=0; j<JOB_ENTRIES; j++) {
		if(jobs[j].pid != 0) {
			if(jobs[j].last_fg > 0 && jobs[j].last_fg < lowest) {
					lowest = jobs[j].last_fg;
					found = j;
			}
		}
	}

	if(found != -1)
		default_bg_job = found;

}

get_second_bg_job() {
	int j;
	int found;
	int lowest;

	lowest = JOB_ENTRIES;
	found = -1;

	for(j=0; j<JOB_ENTRIES; j++) {
		if(jobs[j].pid != 0) {
			if(
				jobs[j].last_fg > 0 &&
				jobs[j].last_fg < lowest &&
				j != default_bg_job
			) {
					lowest = jobs[j].last_fg;
					found = j;
			}
		}
	}

	if(found != -1)
		return(found);
	else
		return(-1);
}

number_of_jobs() {
	int j;
	int count;

	count = 0;

	for(j=0; j<JOB_ENTRIES; j++) {
		if(jobs[j].pid != 0 || jobs[j].pids != NULL) {
			count++;
		}
	}

	return(count);
}

hist_entry_live(hist)
int hist;
{
	int j;

	for(j=0; j<JOB_ENTRIES; j++) {
		if(jobs[j].pid != 0 || jobs[j].pids != NULL) {
			if(jobs[j].history == hist) return(1);
		}
	}

	return(0);
}

change_history_index(original,new)
int original, new;
{
	int j;

	for(j=0; j<JOB_ENTRIES; j++) {
		if(jobs[j].pid != 0 || jobs[j].pids != NULL) {
			if(jobs[j].history == original) {
				jobs[j].history = new;
				return;
			}
		}
	}
}


