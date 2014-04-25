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

env.c:

	functions related to environment variables

*/




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fnmatch.h>


#ifdef LINUX
#endif

#ifdef BSD
#endif


#include "mpsh.h"
#include "env.h"
#include "version.h"

struct env_list *global_env;
struct env_list *set_env();

struct internal_variables {
	char	*name;
	char 	*def;
	int		(*fn)();
} ;

int update_prompt_string();
int update_history_setting();
int update_cdhistory_setting();
int update_error_level();
int update_umask();
int update_nice();
int update_null();

struct internal_variables int_vars[] = {
	"mpsh-version",		mpsh_version,	update_null,
	"mpsh-prompt",		"mpsh$ ",		update_prompt_string,
	"mpsh-history",		"true",			update_history_setting,
	"mpsh-cdhistory",	"true",			update_cdhistory_setting,
	"mpsh-error-level",	"1",			update_error_level,
	"mpsh-umask",		"777",			update_umask,
	"mpsh-nice",		"10",			update_nice,
	"mpsh-hist-disp",	"nc",			update_null,
	"mpsh-hist-disp-l",	"nusxc",		update_null,
	"mpsh-jobs-disp",	"anrcm",		update_null,
	"mpsh-jobs-disp-l",	"aeRhfcm",		update_null,
	"mpsh-eof-exit",	"true",			update_null,
	"mpsh-exp-nl",		"false",		update_null,
	NULL, NULL, NULL
} ;

extern char *prompt_string;

init_global_env(env)
char *env[];
{
	int i;

	global_env = NULL;

	for(i=0; env[i]; i++)
		set_env(env[i]);
}

init_internal_env() {
	int i;
	char *buff;
	mode_t um;
	struct env_list *e;
	int len;

	/* Set mpsh-umask to actual umask inherited from parent process. */
	um = umask(0022);
	umask(um);

	for(i=0; int_vars[i].name; i++) {
		len = strlen(int_vars[i].name) +
			strlen(int_vars[i].def) + 2;
		buff = (char *) malloc(len);
		if(strcmp(int_vars[i].name,"mpsh-umask") == 0) 
			sprintf(buff,"mpsh-umask=%03o",um);
		else
			sprintf(buff,"%s=%s",int_vars[i].name,int_vars[i].def);
		e = set_env(buff);

		/* mpsh-version should be read-only, after being initially set */
		if(strcmp(int_vars[i].name,"mpsh-version") == 0)
			e->flags ^= ENV_FLAG_READONLY;
		
		free(buff);
	}

}

/* Is this environment variable for children, or just internal? */
public_env(str)
char *str;
{
	struct env_list *e;

	e = find_env_var(str);

	if(!e) return(0);

	if(e->flags & ENV_FLAG_INHERIT)
		return(1);
	
	return(0);
}

display_env(env_type,quoted)
int env_type;
int quoted;
{
	struct env_list *e;
	char *pt;
	char buff[64];
	int i;
	int width;

	if(!quoted) {
		width = 0;
		for(e=global_env; e; e=e->next) {
			pt = e->var;

			if(e->flags & env_type) {
				for(i=0; pt[i]; i++)
					if(pt[i] == '=') break;
				if(i > width) width = i;
			}
		}
	}

	for(e=global_env; e; e=e->next) {
		pt = e->var;

		if(e->flags & env_type) {
			for(i=0; pt[i]; i++)
				if(pt[i] == '=') break;
			strncpy(buff,pt,i);
			pt += i;
			if(*pt) pt++;
			buff[i] = '\0';
			if(quoted)
				printf("%s=\"%s\"\n",buff,pt);
			else
				printf("%-*s = %s\n",width,buff,pt);
		}
	}
}


update_umask(str)
char *str;
{
	int u;

	sscanf(str,"%o",&u);
	umask(u);

	return(1);
}

update_nice(str)
char *str;
{
	if(atoi(str) < 1) {
		report_error("mpsh-nice should be an integer greater than 0",str,0,0);
		return(0);
	}
	return(1);
}

update_null(str)
char *str;
{
	return(1);
}

insert_parse(curr,w,str,str2)
struct command *curr;
struct word_list *w;
char *str, *str2;
{
	struct word_list *save_args;
	struct command *further_commands;
	struct word_list *last;
	char *append;

	append = strdup(str2);

	/* Save args & pipeline for appending later. */
	save_args = w->next;
	w->next = NULL;
	further_commands = curr->pipeline;
	curr->pipeline = NULL;

	set_parse_state_word(curr);

	if(str) {
		curr = insert_parse_string(curr,str);
		if(!curr) return(0);
	}

	last = find_last_word(&curr->words);
	append_string_to_word(last,append);

	free(append);

	curr->pipeline = further_commands;
	last->next = save_args;

	return(1);
}


/* NEW ENV STRUCTS */

struct env_list *set_env(str)
char *str;
{
	struct env_list *e;
	int len;
	char *name, *val, *pt;

	len = 0;
	for(pt=str; *pt; pt++) {
		if(*pt == '=') break;
		len++;
	}
	if(!*pt) {
		report_error("Syntax error",str,0,0);
		return(NULL);
	}

	val = strdup(pt+1);
	name = malloc(len+1);
	strncpy(name,str,len);
	name[len] = '\0';

	if(!check_env_setting(name,val)) return(NULL);

	/* Check to see if it already exists */
	e = find_env_var(name);

	if(e) {
		if(e->flags & ENV_FLAG_READONLY) {
			report_error("Read-only variable",name,0,0);
			return(NULL);
		}
		e->flags = 0x0000;
		free(e->var);
	} else {
		e = alloc_env_var();
	}

	e->var = strdup(str);
	e->len = len;
	e->flags = ENV_FLAG_INHERIT; /* default */

	if(strncmp(name,"mpsh-",5) == 0) {
		/* Leave all of this to settings fn ??? */
		e->flags |= ENV_FLAG_SETTING;
		e->flags |= ENV_FLAG_NODELETE;
		e->flags &= ENV_NOT_INHERITED;
	}

	if(strncmp(name,"handler-",8) == 0) {
		e->flags |= ENV_FLAG_HANDLER;
		e->flags &= ENV_NOT_INHERITED;
	}

	if(val[0] == '!') {
		e->flags |= ENV_FLAG_EXEC;
		e->flags &= ENV_NOT_INHERITED;
		if(!(e->flags & ENV_FLAG_TYPE))
			e->flags |= ENV_FLAG_ALIAS;
	}

	if(strcmp(name,"PATH") == 0) update_search_path();

	return(e);
}

struct env_list *alloc_env_var() {
	struct env_list *e;

	if(global_env) {
		for(e=global_env; e->next; e=e->next) ;
		e->next = (struct env_list *) malloc(sizeof(struct env_list));
		e = e->next;
	} else {
		e = global_env =
			(struct env_list *) malloc(sizeof(struct env_list));
	}
	e->var = NULL;
	e->next = NULL;
	e->flags = 0x00;
	e->len = 0;

	return(e);
}

struct env_list *find_env_var(str)
char *str;
{
	struct env_list *e;
	int len;

	len = strlen(str);

	for(e=global_env; e; e=e->next)
		if(e && len == e->len && strncmp(str,e->var,len) == 0) return(e);

	return(NULL);
}

char *get_env(str)
char *str;
{
	struct env_list *e;
	char *pt;

	e = find_env_var(str);
	if(e) {
		pt = e->var + e->len + 1;
		if(e->flags & ENV_FLAG_EXEC)
			return(read_from_command(pt+1));
		return(strdup(pt));
	}
	return(NULL);
}


delete_env(src)
char *src;
{
	int len;
	struct env_list *e, *last;

	len = strlen(src);
	last = NULL;

	for(e=global_env; e; e=e->next) {
		if(e->len == len && strncmp(src,e->var,len) == 0) {
			if(e->flags & ENV_FLAG_NODELETE) {
				report_error("Undeletable variable",src,0,0);
				return;
			}
			if(last)
				last->next = e->next;
			else
				global_env = e->next;
			free(e->var);
			free(e);
			return;
		}
		last = e;
	}
	report_error("Variable not found",src,0,0);
}

expand_env(curr)
struct command *curr;
{
	struct word_list *w;
	char *buff;
	char *val;
	char *pt;
	int i, len;
	int pre;
	char *orig;


	buff = (char *) malloc(BUFF_SIZE);

	while(curr) {
		w = curr->words;
		while(w) {
			retry:
			orig = w->word;
			for(pre=0; orig[pre]; pre++) {
				pt = orig + pre;
				if(*pt == '$') {
					/* Expand env variable. */
					len = strlen(pt+1);
					for(i=len; i; i--) {
						strncpy(buff,pt+1,i);
						buff[i] = '\0';
						val = get_env(buff);
						if(val == (char *) -1) return(0);
						if(val) {
							*pt = '\0';
							if(!insert_parse(curr,w,val,pt+1+i)) return(0);
							goto retry; 
						}
					}
				}
			}
			next:
			w = w->next;
		}
		curr = curr->pipeline;
	}

	free(buff);
	return(1);
}


check_env_setting(name,val)
char *name, *val;
{
	int i;

	if(strncmp(name,"mpsh-",5)) return(1);

	for(i=0; int_vars[i].name; i++) {
		if(strcmp(name,int_vars[i].name) == 0) {
			return((*(int_vars[i].fn))(val));
		}
	}

	return(0);
}


char **build_env(command) 
struct command *command;
{
	int len, i;
	char **call_env;
	struct env_list *e;

	len=0;
	for(e=global_env; e; e=e->next) 
		if(e->flags & ENV_FLAG_INHERIT) len++;

	len += 2;

	call_env = (char **) malloc(sizeof(char *)*len);

	i=0;
	for(e=global_env; e; e=e->next) {
		if(e->flags & ENV_FLAG_INHERIT) {
			call_env[i] = e->var;
			i++;
		}
	}

	call_env[i] = NULL;

	return(call_env);
}

clear_env(arg)
char *arg;
{
	struct env_list *e;
	int env_type;
	char buff[64], *src, *dst;

	env_type = 0x00;

	if(strcmp(arg,"-c") == 0) env_type = ENV_FLAG_INHERIT;
	if(strcmp(arg,"-ca") == 0) env_type = ENV_FLAG_ALIAS;
	if(strcmp(arg,"-ch") == 0) env_type = ENV_FLAG_HANDLER;
	if(strcmp(arg,"-ci") == 0) env_type = ENV_FLAG_SETTING;

	e = global_env;

	while(e) {
		if((e->flags & ENV_FLAG_TYPE) == env_type) {
			if(env_type == ENV_FLAG_SETTING) e->flags ^= ENV_FLAG_NODELETE;
			src = e->var;
			dst = buff;
			while(*src != '=')
				*dst++ = *src++;
			*dst = '\0';
			delete_env(buff);
		} 
			e = e->next;
	}


	if(strcmp(arg,"-ci") == 0) 
		init_internal_env();

}


