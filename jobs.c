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
#include <fcntl.h>


#ifdef Solaris
#include <wait.h>
#endif

#ifdef IRIX
#include <wait.h>
#endif

#ifdef LINUX
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif

#ifdef BSD
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif




#include "mpsh.h"

#include "history.h"
extern struct history_entry **history;



struct job {
	int display;
	char *text;
	char *name;
	pid_t pid;
	pid_t *pids;
	int history;
	int running;
	int smp_id;
	int smp;
	int last_fg;
} ;


#define JOB_ENTRIES 1024
struct job jobs[JOB_ENTRIES];

int current_job_pid;
int default_bg_job;


int control_term;



init_jobs() {
	int i;

	for(i=0; i<JOB_ENTRIES; i++) {
		jobs[i].display = 0;
		jobs[i].pid = 0;
		jobs[i].pids = NULL;
		jobs[i].smp_id = 0;
		jobs[i].last_fg = 0;
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

	jobs[j].display = 1;
	jobs[j].text = strdup(command->text);
	jobs[j].name = NULL;
	jobs[j].pid = command->pid;
	jobs[j].history = add_history_entry(command);
	jobs[j].running = 1;
	jobs[j].last_fg = 0;
	jobs[j].smp = command->smp_num;

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

	jobs[j].display = 1;
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

show_jobs(arg,job)
char *arg;
int job;
{
	char *fmt;
	int *widths;

	if(arg == NULL) {
		fmt = get_env("mpsh-jobs-disp");
	} else {
		if(strcmp(arg,"-s") == 0) 
			fmt = get_env("mpsh-jobs-disp");
		else if(strcmp(arg,"-l") == 0) 
			fmt = get_env("mpsh-jobs-disp-l");
		else {
			if(arg[0] == '-') {
				report_error("Unknown jobs option",arg,0,0);
				return(1);
			}
			fmt = arg;
		}
	}

	widths = (int *) malloc(sizeof(int)*strlen(fmt));

	if(fmt[0] == '\0') {
		report_error("Null jobs format string",NULL,0,0);
		return;
	}

	if(show_jobs_list(fmt,widths,1,job))
		show_jobs_list(fmt,widths,0,job);

	free(widths);
}

delete_job(j)
int j;
{
	jobs[j].display = 0;
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

	if(jobs[j].name) {
		free(jobs[j].name);
		jobs[j].name = NULL;
	}

	return;
}

#if defined BSD || defined LINUX

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
	if(isatty(fileno(stdout)))
		set_terminal(0);


	current_job_pid = -1;

	if(WIFSIGNALED(st)) {
		puts("Killed");
		ret = -1;
	} 

	if(WIFSTOPPED(st)) {
		printf("Job %s stopped\n",
			jobs[job].name ? jobs[job].name : jobs[job].text);
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
							if(jobs[job].display)
								fprintf(stderr,"Job %s %s.\n",
								jobs[job].name ? jobs[job].name :
								jobs[job].text,
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
					if(jobs[job].display)
						fprintf(stderr,"Job %s %s.\n",
							jobs[job].name ? jobs[job].name : jobs[job].text,
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
	if(isatty(fileno(stdout)))
		set_terminal(0);

	current_job_pid = -1;

	if(infop.si_code == CLD_KILLED || infop.si_code == CLD_DUMPED) {
		puts("Killed");
		ret = -1;
	}

	if(infop.si_code == CLD_STOPPED) {
		/*
		puts("Stopped");
		*/
		printf("Job %s stopped\n",
			jobs[job].name ? jobs[job].name : jobs[job].text);
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
							if(jobs[job].display)
								fprintf(stderr,"Job %s %s.\n",
									jobs[job].name ? 
									jobs[job].name : jobs[job].text,
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
				if(jobs[job].display)
					fprintf(stderr,"Job %s %s.\n",
						jobs[job].name ? jobs[job].name : jobs[job].text,
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

#endif

check_for_job_wake(dp)
int dp;
{
	int pid;
	int job;

	pid = dp;
	for(job=0; job<JOB_ENTRIES; job++) {
		if(jobs[job].pid == pid) {
			jobs[job].running = 1;
			change_history_status(jobs[job].history,"Running");
			wait_job(job);
			return;
		}
	}

#ifdef TESTING
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
#endif
}

fg_job(arg)
char *arg;
{
	pid_t pid;

	if(arg) {
		pid = (pid_t) atoi(arg);
	} else {
		if(default_bg_job == -1) find_bg_job();
		if(default_bg_job == -1) {
			report_error("No default job",arg,0,0);
			return(0);
		}
		if(jobs[default_bg_job].pid == 0) {
			report_error("No default job",arg,0,0);
			return(0);
		}
		pid = jobs[default_bg_job].pid;
	}

	if(pid > 0) {
		signal(SIGTTOU,SIG_IGN);
		set_terminal(pid);
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
	char *buff;
	char buff_short[32];
	int i;
	char *pt, *arg;

	buff = buff_short;

	while(curr) {
		w = curr->words;
		while(w) {
			if(w->word && w->word[0] == '%') {
				pt = w->word;
				arg = index(pt,'.');
				if(arg) *arg++ = '\0';
				/* Expand job number. */
				if(pt[1] == '%') { /* default job */
					job = default_bg_job;
				} else if(pt[1] == '-') { /* next default job */
					job = get_second_bg_job();
				} else if(isdigit(pt[1])) {
					job = atoi(pt+1);
				} else {
					job = find_job_by_name(pt+1);
				}
				if(job != -1 && jobs[job].pid != 0 && jobs[job].display) {
					if(!arg && jobs[job].pids) {
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
						if(arg) {
							if(strcmp(arg,"hist") == 0)
								sprintf(buff,"%d",jobs[job].history);
							if(strcmp(arg,"dir") == 0)
								buff = history[jobs[job].history]->dir;
							if(strcmp(arg,"text") == 0)
								buff = jobs[job].text;
							if(strcmp(arg,"name") == 0) {
								buff = jobs[job].name;
								if(!buff) buff = "";
							}
						} else
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
	struct job *jp;

	len = strlen(str);

	/* Check for name match, or strict command match first */
	for(job=0; job<JOB_ENTRIES; job++) {
		jp = &jobs[job];
		if(jp->pid && jp->display) {
			if(jp->name && strncmp(str,jp->name,len) == 0) return(job);
			if(strncmp(str,jp->text,len) == 0) return(job);
		}
	}

	/* Now search for looser command substring match */
	for(job=0; job<JOB_ENTRIES; job++) {
		jp = &jobs[job];
		if(jp->pid != 0 && jp->display) {
			cmp = jp->text;
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
	int pid;
	char *arg;

	if(command->words->next)
		arg = command->words->next->word;
	else
		arg = NULL;

	if(arg && strcmp(arg,"-h") == 0) {
		if(which == PARENT) return(0);
		else {
			puts("usage: wait      # Wait for all jobs to exit");
			puts("       wait %job # Wait for a particular job to exit");
			return(1);
		}
	}

	if(arg && !isdigit(arg[0])) {
		report_error("Syntax error",arg,0,0);
		return(1);
	}

	if(arg) {
		pid = atoi(arg);
		if(pid == 0) {
			report_error("Unknown job",arg,0,0);
			return(1);
		}
		for(j=0; j<JOB_ENTRIES; j++) {
			if(jobs[j].pid == pid && jobs[j].display) {
				while(jobs[j].pid != 0)
					check_for_job_exit();
				return(1);
			}
		}
		report_error("Unknown job",arg,0,0);
	} else {
		for(j=0; j<JOB_ENTRIES; j++) {
			while(jobs[j].pid != 0 && jobs[j].display)
				check_for_job_exit();
		}
	}
	return(1);
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
		if(jobs[j].pid != 0 && jobs[j].display) {
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
		if(jobs[j].pid != 0 && jobs[j].display) {
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
			if(jobs[j].display)
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

show_detailed_job(j)
int j;
{
	struct history_entry *hp;
	struct job *jp;
	time_t tt;
	char buff[32];
	int i;

	if(j == -1) return;
	if(!jobs[j].display) return;

	jp = &jobs[j];
	hp = history[jp->history];
	printf("Job Entry:     %d\n",j);
	printf("History:       %d\n",jp->history);

	if(jp->pids) {
		printf("Process ID:    ");
		for(i=0; jp->pids[i]; i++)
			printf("%d ",jp->pids[i]);
		puts("");
	} else printf("Process ID:    %d\n",jp->pid);

	if(jp->name)
		printf("Name:          %s\n",jp->name);

	printf("Command:       %s\n",jp->text);
	printf("Directory:     %s\n",hp->dir);
	printf("Status:        %s\n",jobs[j].running ? "Running":"Stopped");
	printf("Foreground:    %d\n",jp->last_fg);
	printf("Started:       %s\n",hp->time_start);
	time(&tt);
	format_time(tt - hp->timestamp,buff);
	printf("Elapsed:       %s\n",buff);

	if(jobs[j].pids) {
		for(i=0; jobs[j].pids[i]; i++) ;
		printf("Multi-Process: %d/%d\n",i,jp->smp);
	} else {
		printf("Multi-Process: 1/1\n");
	}

}

show_jobs_list(fmt,widths,calculate,job)
char *fmt;
int *widths;
int calculate;
int job;
{
	int i;
	struct history_entry *hp;
	struct job *jp;
	char buff[256];
	int len;
	int field;
	int fields;
	char *pt;
	char *output;
	int w;
	time_t tt;
	int j;
	int second_bg_job;

	second_bg_job = get_second_bg_job();

	fields = strlen(fmt);

	if(calculate) {
		output = NULL;
		for(field=0; field<fields; field++) 
			widths[field] = 0;
	} else { 
		/* Allocate output buffer based on widths of all fields */
		len = 0;
		for(i=0; i<fields; i++) len += widths[i] +1;
		output = malloc(len+2);
	}

	/*

		n - job number
		h - history number
		c - command
		C - command 20 char
		a - name
		e - elapsed
		t - start time
		d - directory
		D - directory 20 char
		s - one-letter status
		S - long status
		f - one-letter fg default
		F - long fg default
		m - smp count (optional)
		M - smp count
		p - pid

	*/

	for(j=0; j<JOB_ENTRIES; j++) {
		if(job == -1 || job == j)
		if(jobs[j].display) {
			jp = &jobs[j];
			hp = history[jp->history];
			if(!calculate) output[0] = '\0';
			for(field=0; field<fields; field++) {
				switch(fmt[field]) {
					case 'n':
						sprintf(buff,"%d",j);
						pt = buff;
						break;
					case 'h':
						sprintf(buff,"%d",jp->history);
						pt = buff;
						break;
					case 'c':
					case 'C':
						pt = jp->text;
						break;
					case 'a':
						pt = jp->name;
						if(!pt) pt = "";
						break;
					case 'r':
						/* status */
						sprintf(buff,"%c%c",
							j == default_bg_job ? '+' :
							j == second_bg_job ? '-' : ' ',
							jobs[j].running ? 'R' : 'S');
						pt = buff;
						break;
					case 'R':
						/* longer status */
						pt = jobs[j].running ? "Running" : "Stopped";
						break;
					case 'e':
						time(&tt);
						format_time(tt - hp->timestamp,buff);
						pt = buff;
						break;
					case 't':
						sprintf(buff,"%.8s",hp->time_start+11);
						pt = buff;
						break;
					case 'd':
					case 'D':
						pt = hp->dir;
						break;
					case 'f': /* fg status */
						sprintf(buff,"%c",
							j == default_bg_job ? '+' :
							j == second_bg_job ? '-' : ' ');
						pt = buff;
						break;
					case 'F': /* full fg status number */
						sprintf(buff,"%d",jp->last_fg);
						pt = buff;
						break;
					case 'm': /* optional smp numbers */
						if(jobs[j].pids) {
							for(i=0; jobs[j].pids[i]; i++) ;
							sprintf(buff,"(%d/%d)",i,jp->smp);
							pt = buff;
						} else {
							pt = "";
						}
						break;
					case 'M': /* smp numbers */
						if(jobs[j].pids) {
							for(i=0; jobs[j].pids[i]; i++) ;
							sprintf(buff,"%d/%d",i,jp->smp);
							pt = buff;
						} else {
							pt = "1/1";
						}
						break;
					case 'p': /* PID */
						sprintf(buff,"%d",jp->pid);
						pt = buff;
						break;
					default:
						report_error("Unknown jobs field",NULL,fmt[field],0);
						return(0);
				}
				if(calculate) {
					w = strlen(pt);
					if(w > widths[field]) widths[field] = w;
				} else {
					cat_jobs_output(fmt[field],widths[field],output,pt);
				}
			}
			if(!calculate) {
				puts(output);
			}
		}
	}

	if(output) free(output);
	return(1);
}

cat_jobs_output(f,w,out,pt)
int f;
int w;
char *out;
char *pt;
{
	char *output;

	output = out+strlen(out);

	if(out != output) {
		*output++ = ' ';
		*output = '\0';
	}

	switch(f) {
		/* Left justified, any length: */
		case 'c':
			sprintf(output,"%s",pt);
			break;
		/* Left justified, limited to 20 char: */
		case 'C':
			if(w < 20)
				sprintf(output,"%-*s",w,pt);
			else
				sprintf(output,"%-20.20s",pt);
			break;
		case 'D':
			if(w < 20)
				sprintf(output,"%-*s",w,pt);
			else {
				int len;
				len = strlen(pt) - 20;
				if(len < 0) len = 0;
				sprintf(output,"%-20.20s",pt+len);
			}
			break;
		/* Left justified: */
		case 'd':
		case 'a':
			sprintf(output,"%-*s",w,pt);
			break;
		/* Right justified */
		default:
			sprintf(output,"%*s",w,pt);
			break;
	}
}

delete_job_entry(arg)
char *arg;
{
	int j;
	int pid;
	int i;

	pid = atoi(arg);

	for(j=0; j<JOB_ENTRIES; j++) {
		if(jobs[j].display) {
			if(jobs[j].pids) {
				for(i=0; jobs[j].pids[i]; i++)
					if(jobs[j].pids[i] == pid) {
						jobs[j].display = 0;
						return;
				}
			} else if(jobs[j].pid == pid) {
				jobs[j].display = 0;
				return;
			}
		}
	}

	report_error("Job not found",arg,0,0);
}

pid_to_job(arg)
char *arg;
{
	int pid;
	int job;
	int i;

	pid = atoi(arg);

	for(job=0; job<JOB_ENTRIES; job++) {
		if(jobs[job].pid) {
			if(jobs[job].pid == pid) {
				if(jobs[job].display)
					return(job);
				else
					return(-1);
			}
		}
		if(jobs[job].pids) {
			/* Search through all PIDs */
			for(i=0; jobs[job].pids[i]; i++)
				if(jobs[job].pids[i] == pid)
					return(job);
		}
	}
	return(-1);
}

name_job(job,name)
int job;
char *name;
{
	if(jobs[job].name) free(jobs[job].name);
	jobs[job].name = strdup(name);
}


set_terminal(pid)
int pid;
{
	tcsetpgrp(control_term,getpgid(pid));
}

init_terminal() {
	control_term = open("/dev/tty",O_RDONLY,0x00);

	if(control_term == -1)
		report_error("Error opening device","/dev/tty",0,1);
}


