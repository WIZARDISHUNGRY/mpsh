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

history.c:

	command history functions

*/




#include <stdio.h>
#include <strings.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/types.h>
#include <signal.h>


#ifdef LINUX
#include <stdlib.h>
#include <string.h>
#endif

#ifdef BSD
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif



#include "mpsh.h"
#include "state.h"
#include "signal_names.h"


struct history_entry {
	char *text;
	time_t user_time;
	time_t sys_time;
	char time_start[28];
	char time_end[28];
	int status;
	int signo;
	int code;
	int errno;
	int smp_id;
} ;

#define HISTORY_SIZE 1024
struct history_entry history[HISTORY_SIZE];

int history_next;

int show_history_sub;

char *format_time();

init_history() {
	int i;

	/*
	show_history_sub = 0;
	*/

	for(i=0; i<HISTORY_SIZE; i++)
		history[i].text = NULL;

	history_next = 0;
}


int add_history_entry(command)
struct command *command;
{
	int hist;
	struct history_entry *h;
	time_t tt;
	int len;
	int i;

	if(command->pipeline && !command->smp_num) return(-1);

	len = strlen(command->text);
	while(len > 1 && command->text[len-1] == ' ') {
		command->text[len-1] = '\0';
		len = strlen(command->text);
	}

	if(command->smp_id)
		for(i=0; i<HISTORY_SIZE; i++)
			if(history[i].smp_id == command->smp_id) return(-1);


	hist = history_next;
	history_next++;
	h = &history[hist%HISTORY_SIZE];
	h->text = strdup(command->text);
	h->user_time = h->sys_time = 0L;
	h->status = 0x00;
	h->signo = 0x00;
	h->errno = 0x00;

	time(&tt);
	strcpy(h->time_start,ctime(&tt));
	h->time_start[strlen(h->time_start)-1] = '\0';
	strcpy(h->time_end,"Running");

	h->smp_id = command->smp_id;

	return(hist);
}

add_history_builtin(command)
struct command *command;
{
	int hist;
	struct history_entry *h;
	time_t tt;
	int len;
	int i;

	if(command->pipeline && !command->smp_num) return(-1);

	len = strlen(command->text);
	while(len > 1 && command->text[len-1] == ' ') {
		command->text[len-1] = '\0';
		len = strlen(command->text);
	}

	hist = history_next;
	history_next++;
	h = &history[hist%HISTORY_SIZE];
	h->text = strdup(command->text);
	h->user_time = h->sys_time = 0L;
	h->status = 0x00;
	h->signo = 0x00;
	h->errno = 0x00;
	h->code = CLD_EXITED;

	time(&tt);
	strcpy(h->time_start,ctime(&tt));
	h->time_start[strlen(h->time_start)-1] = '\0';
	strcpy(h->time_end,ctime(&tt));
	h->time_end[strlen(h->time_end)-1] = '\0';

	h->smp_id = command->smp_id;

	return(hist);
}

#ifdef BSD

add_history_details(hist,st,ruse) /* BSD VERSION */
int hist;
int st;
struct rusage *ruse;
{
	struct history_entry *hp;
	int div;
	time_t tt;

	if(hist == -1) return;

	hp = &history[hist%HISTORY_SIZE];

	if(ruse) {
		hp->signo = WTERMSIG(st);
		hp->code = WIFEXITED(st) ? CLD_EXITED : 0;
		hp->errno = 0;

		if(WIFEXITED(st)) { /* Child exit, CPU usage stats available */
			hp->user_time += ruse->ru_utime.tv_sec;
			hp->sys_time += ruse->ru_stime.tv_sec;
			hp->status = WEXITSTATUS(st);
		} else
			hp->status = WIFSIGNALED(st);
	}

	time(&tt);
	strcpy(hp->time_end,ctime(&tt));
	hp->time_end[strlen(hp->time_end)-1] = '\0';
}

add_history_time(hist,st,ruse) /* BSD VERSION */
int hist;
int st;
struct rusage *ruse;
{
	struct history_entry *hp;
	int div;
	time_t tt;

	if(hist == -1) return;

	hp = &history[hist%HISTORY_SIZE];

	if(ruse) {
		if(WIFEXITED(st)) { /* Child exit, CPU usage stats available */
			hp->user_time += ruse->ru_utime.tv_sec;
			hp->sys_time += ruse->ru_stime.tv_sec;
		} 
	}
}

#else

add_history_details(hist,infop) /* NON-BSD VERSION */
int hist;
siginfo_t *infop;
{
	struct history_entry *hp;
	int div;
	time_t tt;

	if(hist == -1) return;

	/* Is this portable? */
	div = sysconf(_SC_CLK_TCK);

	hp = &history[hist%HISTORY_SIZE];

	if(infop) {
		hp->signo = infop->si_signo;
		hp->code = infop->si_code;
		hp->errno = infop->si_errno;

		if(hp->signo == SIGCLD) { /* Child exit, CPU usage stats available */
			hp->user_time += infop->si_utime/div;
			hp->sys_time += infop->si_stime/div;
			hp->status = infop->si_status;
		}
	}

	time(&tt);
	strcpy(hp->time_end,ctime(&tt));
	hp->time_end[strlen(hp->time_end)-1] = '\0';
}


add_history_time(hist,infop) /* NON-BSD VERSION */
int hist;
siginfo_t *infop;
{
	struct history_entry *hp;
	int div;
	time_t tt;

	if(hist == -1) return;

	/* Is this portable? */
	div = sysconf(_SC_CLK_TCK);

	hp = &history[hist%HISTORY_SIZE];

	if(infop) {
		if(infop->si_signo == SIGCLD) { /* CPU usage stats available */
			hp->user_time += infop->si_utime/div;
			hp->sys_time += infop->si_stime/div;
		}
	}
}

#endif

show_history(arg) 
char *arg;
{
	int details;
	int i;
	int h;
	struct history_entry *hp;
	char buff1[32], buff2[32];
	int user_widest, sys_widest, exit_widest;
	int len;

	if(arg == NULL) {
		details = 0;
	} else {
		if(strcmp(arg,"-l") == 0) {
			details = 1;
			arg = NULL;
		}
	}

	if(arg != NULL) {
		if(isdigit(arg[0])) {
			hp = &(history[atoi(arg)%HISTORY_SIZE]);
			if(hp->text) {
				printf("Entry:       %d\n",atoi(arg));
				printf("Started:     %s\n",hp->time_start);
				printf("Ended:       %s\n",hp->time_end);
				printf("User CPU:    %s\n",format_time(hp->user_time,buff1));
				printf("System CPU:  %s\n", format_time(hp->sys_time,buff2));

				if(hp->code == CLD_EXITED)
					printf("Exit Status: %d\n",hp->status);
				else
					printf("Killed by:   %s\n",signal_names[hp->status]);

				printf("Command:     %s\n",hp->text);
			} else {
				report_error("Unknown history entry",arg,0,0);
			}
			return;
		} else {
			report_error("Unknown history option",arg,0,0);
			return;
		}
	}

	if(details == 0) {
		printf(" Num Command\n");
		for(i=0; i<HISTORY_SIZE; i++) {
			h = history_next+i-1024;
			if(h >= 0) {
				hp = &(history[h%HISTORY_SIZE]);
				if(hp->text) {
					printf("%4d %s\n",h,hp->text);
				}
			}
		}
	} else {
		/* Find widest time strings */
		user_widest = 4;
		sys_widest = 3;
		exit_widest = 4;
		for(i=0; i<HISTORY_SIZE; i++) {
			h = history_next+i-1024;
			if(h >= 0) {
				hp = &(history[h%HISTORY_SIZE]);
				if(hp->text) {
					len = strlen(format_time(hp->user_time,buff1));
					if(len > user_widest) user_widest = len;
					len = strlen(format_time(hp->sys_time,buff2));
					if(len > sys_widest) sys_widest = len;
					if(hp->code != CLD_EXITED) {
						if(strcmp(hp->time_end,"Running") == 0)
							len = 7;
						else
							len = strlen(signal_names[hp->status]);
						if(len > exit_widest) exit_widest = len;
					}
				}
			}
		}

		printf("%4s %*s %*s %*s Command\n",
			"Num",
			user_widest, "User",
			sys_widest, "Sys",
			exit_widest, "Exit");

		for(i=0; i<HISTORY_SIZE; i++) {
			h = history_next+i-1024;
			if(h >= 0) {
				hp = &(history[h%HISTORY_SIZE]);
				if(hp->text) {
					if(hp->code == CLD_EXITED) {
						if(hp->status == 0)
							/* exited ok */
							printf("%4d %*s %*s %*s %-40s\n",h,
								user_widest, format_time(hp->user_time,buff1),
								sys_widest, format_time(hp->sys_time,buff2),
								exit_widest,"ok",
								hp->text);
						else
							/* exited with error code */
							printf("%4d %*s %*s %*d %-40s\n",h,
								user_widest, format_time(hp->user_time,buff1),
								sys_widest, format_time(hp->sys_time,buff2),
								exit_widest,hp->status,
								hp->text);
					} else {
						if(strcmp(hp->time_end,"Running") == 0)
							/* still running */
							printf("%4d %*s %*s %*s %-40s\n",h,
								user_widest, format_time(hp->user_time,buff1),
								sys_widest, format_time(hp->sys_time,buff2),
								exit_widest,"Running",
								hp->text);
						else
							/* killed */
							printf("%4d %*s %*s %*s %-40s\n",h,
								user_widest, format_time(hp->user_time,buff1),
								sys_widest, format_time(hp->sys_time,buff2),
								exit_widest,signal_names[hp->status],
								hp->text);
					}
				}
			}
		}
	}

}


char *format_time(t,buff)
clock_t t;
char *buff;
{
	int sec, min, hour, days;

	/*
	printf("%d\n",t);
	*/

	sec = t%60;
	t = t/60;
	min = t%60;
	t = t/60;
	hour = t%24;
	t = t/24;
	days = t;

	if(days > 0)
		sprintf(buff,"%d:%02d:%02d:%02d",days,hour,min,sec);
	else if(hour > 0)
		sprintf(buff,"%02d:%02d:%02d",hour,min,sec);
	else
		sprintf(buff,"%02d:%02d",min,sec);

	return(buff);
}

struct command *expand_history(command)
struct command *command;
{
	char *pt;
	char *text;
	int i;
	int h;
	int len;

	pt = command->words->word+1;
	len = strlen(pt);

	if(isdigit(pt[0])) {
		h = atoi(pt);
		text = history[h%HISTORY_SIZE].text;
		if(text) {
			command = parse_string(command,text);
			if(show_history_sub) 
				command->display_text = 1;
			return(command);
		}
	}

	for(i=HISTORY_SIZE-1; i>=0; i--) {
		h = history_next+i;
		text = history[h%HISTORY_SIZE].text;
		if(text) {
			if(strncmp(pt,text,len) == 0) {
				command = parse_string(command,text);
				if(show_history_sub) 
					command->display_text = 1;
				return(command);
			}
		}
	}
	return(command);
}

struct command *parse_string(command,text)
struct command *command;
char *text;
{
	int state;
	char *str;
	struct command *ret;

	command->words = NULL;
	command->text[0] = '\0';
	command->state = STATE_SPACE;
	for(str=text; *str; str++) {
		add_command_text_letter(command,*str);
		ret = parse_char(command,*str);
		if(!ret) return(0);
		command = ret;
	}
	add_command_text_letter(command,' ');
	command = parse_char(command,' ');
	return(command);
}

update_history_setting(str)
char *str;
{
	show_history_sub = atoi(str);
}

