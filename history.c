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
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>


#ifdef LINUX
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
	char *expansion;
	char *dir;
	char exit[16];
	time_t user_time;
	time_t sys_time;
	time_t timestamp;
	time_t elapsed;
	char time_start[28];
	char time_end[28];
	int smp_id;
} ;

#define HISTORY_ALLOC_SIZE 1024
int hist_allocated;
struct history_entry **history;

int history_next;

int show_history_sub;

char *format_time();

init_history() {
	int i;

	history = (struct history_entry **) 
		malloc(sizeof(struct history_entry *)*HISTORY_ALLOC_SIZE);
	hist_allocated = HISTORY_ALLOC_SIZE;

	for(i=0; i<hist_allocated; i++) {
		history[i] = NULL;
	}

	history_next = 0;
}

clear_history() {
	int i;
	int tmp_entry;
	int jobs;
	struct history_entry *hp;
	struct history_entry **hist_tmp;

	jobs = number_of_jobs();
	hist_tmp = (struct history_entry **) 
		malloc(sizeof(struct history_entry *)*jobs);
	tmp_entry = 0;

	for(i=0; history[i]; i++) {
		/* Any history entry for a jobs that's still running
		   must be saved via the hist_tmp array.
		*/
		if(hist_entry_live(i)) {
			hist_tmp[tmp_entry] = history[i];
			change_history_index(i,tmp_entry);
			tmp_entry++;
		} else {
			hp = history[i];
			free(hp->text);
			free(hp->expansion);
			free(hp->dir);
			free(hp);
		}
		history[i] = NULL;
	}

	init_history();

	/* Copy back temp array, if any */
	for(tmp_entry=0; tmp_entry<jobs; tmp_entry++)
		history[tmp_entry] = hist_tmp[tmp_entry];

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
		for(i=0; history[i]; i++)
			if(history[i]->smp_id == command->smp_id) return(-1);

	hist = get_new_hist_entry();
	h = history[hist];

	h->text = command->text;
	h->expansion = command->expansion;
	h->dir = command->dir;
	strcpy(h->exit,"Running");
	h->user_time = h->sys_time = 0L;
	h->elapsed = -1;

	time(&tt);
	h->timestamp = tt;
	strcpy(h->time_start,ctime(&tt));
	h->time_start[strlen(h->time_start)-1] = '\0';
	strcpy(h->time_end,"Running");

	h->smp_id = command->smp_id;

	return(hist);
}

get_new_hist_entry() {
	int i;
	int h;

	for(i=0; history[i]; i++) ;

	if(i+1 == hist_allocated) {
		hist_allocated += HISTORY_ALLOC_SIZE;
		history = realloc(history,sizeof(struct history_entry *)*HISTORY_ALLOC_SIZE);
		h = i;
		while(h < hist_allocated) history[h++] = NULL;
	}

	history[i] = (struct history_entry *) 
		malloc(sizeof(struct history_entry));

	return(i);
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

	hist = get_new_hist_entry();
	h = history[hist];

	h->text = command->text;
	h->expansion = command->expansion;
	h->dir = command->dir;
	strcpy(h->exit,"ok");
	h->user_time = h->sys_time = 0L;

	time(&tt);
	h->timestamp = tt;
	h->elapsed = 0;
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
	int code, status;

	if(hist == -1) return;

	hp = history[hist];

	if(ruse) {
		code = WIFEXITED(st) ? CLD_EXITED : 0;

		if(WIFEXITED(st)) { /* Child exit, CPU usage stats available */
			hp->user_time += ruse->ru_utime.tv_sec;
			hp->sys_time += ruse->ru_stime.tv_sec;
			status = WEXITSTATUS(st);
		} else
			status = WIFSIGNALED(st);
	}

	time(&tt);
	strcpy(hp->time_end,ctime(&tt));
	hp->elapsed = tt - hp->timestamp;
	hp->time_end[strlen(hp->time_end)-1] = '\0';

	if(ruse) set_exit_status(hp,code,status);
	else strcpy(hp->exit,"ok");
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

	hp = history[hist];

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
	int code, status;

	if(hist == -1) return;

	/* Is this portable? */
	div = sysconf(_SC_CLK_TCK);

	hp = history[hist];

	if(infop) {
		code = infop->si_code;

		if(infop->si_signo == SIGCLD) { 
			/* Child exit, CPU usage stats available */
			hp->user_time += infop->si_utime/div;
			hp->sys_time += infop->si_stime/div;
			status = infop->si_status;
		}
	}

	time(&tt);
	strcpy(hp->time_end,ctime(&tt));
	hp->elapsed = tt - hp->timestamp;
	hp->time_end[strlen(hp->time_end)-1] = '\0';

	if(infop) set_exit_status(hp,code,status);
	else strcpy(hp->exit,"ok");
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

	hp = history[hist];

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
	char *fmt;
	int *widths;
	time_t tt;

	if(arg == NULL) {
		fmt = get_env("mpsh-hist-disp");
	} else {
		if(strcmp(arg,"-l") == 0) {
			fmt = get_env("mpsh-hist-disp-l");
			arg = NULL;
		}
	}

	if(arg != NULL) {
		if(isdigit(arg[0])) {
			h = atoi(arg);
			for(i=0; history[i]; i++) ;
			if(h >= i) {
				report_error("History entry out of bounds",arg,0,0);
				return;
			}
			hp = history[h];
			if(hp->text) {
				printf("Entry:       %d\n",atoi(arg));
				printf("Started:     %s\n",hp->time_start);
				printf("Ended:       %s\n",hp->time_end);

				if(hp->elapsed != -1) {
					printf("Elapsed:     %s\n",format_time(hp->elapsed,buff1));
				} else {
					time(&tt);
					printf("Elapsed:     %s\n",format_time(
					tt - hp->timestamp,buff1));
				}

				printf("User CPU:    %s\n",format_time(hp->user_time,buff1));
				printf("System CPU:  %s\n", format_time(hp->sys_time,buff2));
				if(hp->dir)
					printf("Directory:   %s\n",hp->dir);

				if(hp->exit[0] == 'S' && hp->exit[1] == 'I')
					printf("Killed by:   %s\n",hp->exit);
				else
					printf("Exit Status: %s\n",hp->exit);

				printf("Command:     %s\n",hp->text);
			} else {
				report_error("Unknown history entry",arg,0,0);
			}
			return;
		} else {
			if(arg[0] == '-') {
				report_error("Unknown history option",arg,0,0);
				return;
			} else {
				fmt = arg;
			}
		}
	}

	widths = (int *) malloc(sizeof(int)*strlen(fmt));

	if(show_history_list(fmt,widths,1))
		show_history_list(fmt,widths,0);

	free(widths);
}

show_history_list(fmt,widths,calculate)
char *fmt;
int *widths;
int calculate;
{
	int details;
	int i;
	int h;
	struct history_entry *hp;
	char buff[256];
	char buff1[32], buff2[32];
	int len;
	int field;
	int fields;
	char *pt;
	char *output;
	int w;
	time_t tt;

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

	/* Header line */
	if(!calculate) output[0] = '\0';
	for(field=0; field<fields; field++) {
		switch(fmt[field]) {
			case 'n':
				pt = "Num";
				break;
			case 'c':
			case 'C':
				pt = "Command";
				break;
			case 'u':
				pt = "User";
				break;
			case 's':
				pt = "Sys";
				break;
			case 'e':
				pt = "Elapsed";
				break;
			case 't':
				pt = "Started";
				break;
			case 'x':
				pt = "Exit";
				break;
			case 'd':
			case 'D':
				pt = "Directory";
				break;
			default:
				report_error("Unknown history field",NULL,fmt[field],0);
				return(0);
		}
		if(calculate) {
			w = strlen(pt);
			if(w > widths[field]) widths[field] = w;
		} else {
			cat_hist_output(fmt[field],widths[field],output,pt);
		}
	}
	if(!calculate) puts(output);


	/* History entries */
	for(h=0; history[h]; h++) {
		hp = history[h];
		if(hp->text) {
			if(!calculate) output[0] = '\0';
			for(field=0; field<fields; field++) {
				switch(fmt[field]) {
					case 'n':
						sprintf(buff,"%d",h);
						pt = buff;
						break;
					case 'c':
					case 'C':
						pt = hp->text;
						break;
					case 'u':
						format_time(hp->user_time,buff),
						pt = buff;
						break;
					case 's':
						format_time(hp->sys_time,buff),
						pt = buff;
						break;
					case 'e':
						if(hp->elapsed != -1) {
							format_time(hp->elapsed,buff);
							pt = buff;
						} else {
							time(&tt);
							format_time(tt - hp->timestamp,buff);
							pt = buff;
						}
						break;
					case 't':
						sprintf(buff,"%.8s",hp->time_start+11);
						pt = buff;
						break;
					case 'x': /* Exit status / signal / etc */
						pt = hp->exit;
						break;
					case 'd':
					case 'D':
						pt = hp->dir;
						break;
				}
				if(calculate) {
					w = strlen(pt);
					if(w > widths[field]) widths[field] = w;
				} else {
					cat_hist_output(fmt[field],widths[field],output,pt);
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

cat_hist_output(f,w,out,pt)
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
		case 'x': 
			sprintf(output,"%-*s",w,pt);
			break;
		/* Right justified */
		default:
			sprintf(output,"%*s",w,pt);
			break;
	}
}


char *format_time(t,buff)
clock_t t;
char *buff;
{
	int sec, min, hour, days;

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
	int text_dir, text_command, text_num;
	int text_parsed;
	char *arg;

	if(parse_depth++ > MAX_PARSE_DEPTH) {
		report_error("Excessive parse depth",NULL,0,0);
		return(NULL);
	}

	text_dir = text_command = text_num = text_parsed = 0;

	pt = command->words->word+1;

	arg = index(pt,'.');
	if(arg && arg[1] != '/') {
		*arg++ = '\0';
		if(strcmp(arg,"dir") == 0) {
			text_dir = 1;
		} else if(strcmp(arg,"text") == 0) {
			text_command = 1;
		} else if(strcmp(arg,"parsed") == 0) {
			text_parsed = 1;
		} else if(strcmp(arg,"num") == 0) {
			text_num = 1;
		} else {
			report_error("History argument unknown",arg,0,0);
			return(NULL);
		}
	}

	h = find_history(pt);

	if(h == -1) {
		return(NULL);
	}

	if(text_dir) {
		command->echo_text = dup_history_dir(h);
		return(command);
	}

	if(text_command) {
		command->echo_text = strdup(history[h]->text);
		return(command);
	}

	if(text_parsed) {
		command->echo_text = strdup(history[h]->expansion);
		return(command);
	}

	if(text_num) {
		char *tmp;
		tmp = malloc(16);
		sprintf(tmp,"%d",h);
		command->echo_text = tmp;
		return(command);
	}

	text = history[h]->text;
	if(text) {
		command = parse_string(command,text);
		if(show_history_sub) 
			command->display_text = 1;
	}
	return(command);
}

find_history(pt)
char *pt;
{
	char *text;
	int i;
	int h;
	int len;
	int anywhere, j;

	if(isdigit(pt[0])) {
		h = atoi(pt);
		for(i=0; history[i]; i++) ;
		if(h >= i) {
			report_error("History entry out of bounds",pt,0,0);
			return(-1);
		}
		return(h);
	}

	if(pt[0] == '*') { /* Match pattern anywhere in history string */
		anywhere = 1;
		*pt = '\0';
		pt++;
	} else {
		anywhere = 0;
	}
	

	len = strlen(pt);

	for(h=0; history[h]; h++) ;

	h--;

	while(h >= 0) {
		text = history[h]->text;
		if(anywhere) {
			for(j=0; text[j]; j++)
				if(strncmp(pt,text+j,len) == 0)
					return(h);
		} else {
			if(strncmp(pt,text,len) == 0) 
				return(h);
		}
		h--;
	}
	report_error("History entry not found",pt,0,0);
	return(-1);
}

update_history_setting(str)
char *str;
{
	show_history_sub = atoi(str);
}

char *dup_history_dir(h) 
int h;
{
	return(strdup(history[h]->dir));
}

set_exit_status(hp,code,status)
struct history_entry *hp;
int code,status;
{
	char *pt;

	pt = hp->exit;

	if(code == CLD_EXITED) {
		if(status == 0) {
			strcpy(pt,"ok");
		} else {
			sprintf(pt,"%d",status);
		}
	} else {
		if(strcmp(hp->time_end,"Running") != 0)
			strcpy(pt,signal_names[status]);
	}
}

change_history_status(h,status)
int h;
char *status;
{
	strcpy(history[h]->exit,status);
}


