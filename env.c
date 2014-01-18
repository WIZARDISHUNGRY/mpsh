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
#include "version.h"

struct internal_variables {
	char	*name;
	char 	*def;
	int		(*fn)();
} ;

int update_version();
int update_prompt_string();
int update_history_setting();
int update_cdhistory_setting();
int update_error_level();
int update_umask();
int update_nice();
int update_null();

struct internal_variables int_vars[] = {
	/* mpsh-version MUST be entry [0] */
	"mpsh-version=",		mpsh_version,	update_version,
	"mpsh-prompt=",			"mpsh$ ",		update_prompt_string,
	"mpsh-history=",		"1",			update_history_setting,
	"mpsh-cdhistory=",		"1",			update_cdhistory_setting,
	"mpsh-error-level=",	"1",			update_error_level,
	"mpsh-umask=",			NULL,			update_umask,
	"mpsh-nice=",			"10",			update_nice,
	"mpsh-hist-disp=",		"nc",			update_null,
	"mpsh-hist-disp-l=",	"nusxc",		update_null,
	"mpsh-eof-exit=",		"1",			update_null,
	NULL, NULL, NULL
} ;

extern char *prompt_string;

struct path_list {
	char *dir;
	char *command;
	struct path_list *next;
} ;

struct path_list *new_path_list();
struct path_list *command_path_list;

init_command_path_list() {
	command_path_list = NULL;
}

init_global_env(env)
char *env[];
{
	int i;
	struct word_list *w;

	w = global_env = init_word();

	for(i=0; env[i]; i++) {
		w->next = init_word_str(env[i]);
		w = w->next;
	}
}

init_internal_env() {
	int i;
	char buff[1024];
	mode_t um;

	for(i=0; int_vars[i].name; i++) {
		if(int_vars[i].def) {
			sprintf(buff,"%s%s",int_vars[i].name,int_vars[i].def);
			set_env_str(buff);
		}
	}

	/* Set mpsh-umask to actual umask inherited from parent process. */
	um = umask(0022);
	umask(um);
	sprintf(buff,"mpsh-umask=%03o",um);
	set_env_str(buff);

}


update_search_path() {
	struct word_list *pth;
	struct path_list *c;
	char *pt;
	char *var;
	struct dirent *dp;
	DIR *dirp;

	if(search_path) 
		free_word_list(search_path);

	pth = search_path = init_word();

	var = pt = get_env("PATH");

	while(*pt) {
		if(*pt == ':') {
			pth->next = init_word();
			pth = pth->next;
		} else {
			add_letter_to_word(pth,*pt);
		}
		pt++;
	}
	free(var);


	/* Update list of commands based on new PATH */
	if(command_path_list) {
		free_path_list(command_path_list);
		command_path_list = NULL;
	}

	for(pth=search_path; pth; pth=pth->next) {
		/* "." is a (very) special case. */
		if(strcmp(pth->word,".") == 0) {
			if(!command_path_list) {
				command_path_list = c =
					new_path_list(pth->word,NULL);
			} else {
				c->next = new_path_list(pth->word,NULL);
				c = c->next;
			}
		} else {
			/* For each entry in search_path, read through directory */
			dirp = opendir(pth->word);
			if(dirp) {
				while(dp = readdir(dirp)) {
					if(!command_path_list) {
						command_path_list = c =
							new_path_list(pth->word,dp->d_name);
					} else {
						c->next = new_path_list(pth->word,dp->d_name);
						c = c->next;
					}
				}
				closedir(dirp);
			}
		}
	}
	return;
}

free_path_list(c)
struct path_list *c;
{
	while(c) {
		if(c->dir) free(c->dir);
		if(c->command) free(c->command);
		free(c);
		c = c->next;
	}
}

struct path_list *new_path_list(dir,command)
char *dir, *command;
{
	struct path_list *c;

	c = (struct path_list *) malloc(sizeof(struct path_list));
	c->next = NULL;
	c->dir = (char *) malloc(strlen(dir)+1);
	strcpy(c->dir,dir);
	if(command) {
		c->command = (char *) malloc(strlen(command)+1);
		strcpy(c->command,command);
	} else {
		c->command = NULL;
	}
	return(c);
}

char *find_path(arg)
char *arg;
{
	struct word_list *w;
	struct path_list *c;
	struct stat st;
	char buff[256];

	if(!arg[0] || arg[0] == '[') {
		return(NULL);
	}

	if(index(arg,'/') != NULL) {
		return(strdup(arg));
	}

	w = search_path;

	for(c=command_path_list; c; c=c->next) {
		if(strcmp(c->dir,".") == 0) { 
			/* Search current directory by hand */
			if(stat(arg,&st) == 0) {
				return(strdup(arg));
			}
		} else {
			if(strcmp(arg,c->command) == 0) {
				sprintf(buff,"%s/%s",c->dir,arg);
				return(strdup(buff));
			}
		}
	}


	/*
		If we don't find the command in our cache, try all PATH
		entries by hand, and if we find a result, update the cache.
	*/

	do {
		sprintf(buff,"%s/%s",w->word,arg);
		if(stat(buff,&st) == 0) {
			update_search_path();
			return(strdup(buff));
		}
		w = w->next;
	} while(w->next);

	return(NULL);
}

set_env(command)
struct command *command;
{
	char *str;

	if(!command->words->next) {
		report_error("Missing setenv option",NULL,0,0);
		return(0);
	}

	str = command->words->next->word;
	set_env_str(str);
}

show_env(command)
struct command *command;
{
	char *str;

	if(!command->words->next) {
		report_error("Missing mpsh-show option",NULL,0,0);
		return(0);
	}

	str = command->words->next->word;

	if(str[0] == '-') {

		if(strcmp(str,"-e") == 0) {
			show_env_public();
			return(1);
		}

		if(strcmp(str,"-c") == 0) {
			show_env_private();
			return(1);
		}

		if(strcmp(str,"-i") == 0) {
			show_env_mpsh();
			return(1);
		}

		report_error("Unknown mpsh-show option",str,0,0);
		return(0);
	}

	report_error("Unknown mpsh-show option",str,0,0);
	return(0);
}

set_env_str(str)
char *str;
{
	int sl;
	struct word_list *w;
	int len;
	int i;

	for(sl=0; str[sl] && str[sl] != '='; sl++) ;
	if(str[sl] == '=') sl++;
	else {
		report_error("setenv syntax error",str,0,0);
		return;
	}

	for(w=global_env->next; w; w=w->next) {
		if(strncmp(str,w->word,sl) == 0) {
			string_to_word(str,&w);
			goto skip;
		}
	}
	append_word(&global_env,init_word_str(str));
	skip:

	if(strncmp("PATH=",str,5) == 0) update_search_path();
	if(strncmp("mpsh-",str,5) == 0) {
		/* mpsh internal setting */
		for(i=0; int_vars[i].name; i++) {
			len = strlen(int_vars[i].name);
			if(strncmp(str,int_vars[i].name,len) == 0) {
				(*(int_vars[i].fn))(str+len);
			}
		}
	}


}

char *get_env(env)
char *env;
{
	char *search;
	char *pt;
	char *ret;
	int len;
	struct word_list *w;

	len = strlen(env);
	search = (char *) malloc(len+2);
	strcpy(search,env);
	search[len] = '=';
	len++;
	search[len] = '\0';

    for(w=global_env->next; w; w=w->next) {
		if(strncmp(search,w->word,len) == 0) {
			pt = w->word+len;
			len = strlen(pt);
			ret = (char *) malloc(len+1);
			strcpy(ret,pt);
			free(search);
			return(ret);
		}
	}
	free(search);
	return(NULL);
}

expand_env(curr)
struct command *curr;
{
	struct word_list *w;
	char buff[256];
	char *val;
	char *pt;
	int i, len;
	char *new;
	int pre;
	char *orig;
	char *tmp;

	while(curr) {
		w = curr->words;
		while(w) {
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
						if(val) {
							if(val[0] == '!') {
								tmp = read_from_command(val+1);
								free(val);
								val = tmp;
							}
							*pt = '\0';
							if(!insert_parse(curr,w,val,pt+1+i)) return(0);
							break;

						}
					}
				}
			}
			w = w->next;
		}
		curr = curr->pipeline;
	}
	return(1);
}

/* Is this environment variable for children, or just internal? */
public_env(e)
char *e;
{
	char *pt;

	/* variable alias: */
	pt = index(e,'=');
	if(pt[1] == '!') {
		return(0);
	}

	/* Internal settings */
	if(strncmp(e,"mpsh-",5) == 0)
		return(0);

	return(1);
}

show_env_public() {
	struct word_list *w;
    for(w=global_env->next; w; w=w->next) 
		if(public_env(w->word))
			puts(w->word);
}

show_env_private() {
	struct word_list *w;
    for(w=global_env->next; w; w=w->next) 
		if(!public_env(w->word) && strncmp(w->word,"mpsh-",5) != 0)
			puts(w->word);
}

show_env_mpsh() {
	struct word_list *w;
    for(w=global_env->next; w; w=w->next) 
		if(!public_env(w->word) && strncmp(w->word,"mpsh-",5) == 0)
			puts(w->word);
}


check_command_glob(w)
struct word_list *w;
{
	struct path_list *c;
	char *arg;
	char buff[256];

	arg = w->word;

	if(!arg) return;

	if(arg[0] == '[') return;

	if(index(arg,'?') != 0 || index(arg,'*') != 0 || index(arg,'[') != 0) {
		for(c=command_path_list; c; c=c->next) {
			if(fnmatch(arg,c->command,FNM_PATHNAME) == 0) {
				sprintf(buff,"%s/%s",c->dir,c->command);
				string_to_word(buff,&w);
				return;
			}
		}
	}
}

update_version(str)
char *str;
{
	char buff[64];
	int len;
	struct word_list *w;

	report_error("Read-only variable",str,0,0);

	sprintf(buff,"%s%s",int_vars[0].name,int_vars[0].def);

	len = strlen(int_vars[0].name);
	for(w=global_env->next; w; w=w->next) {
		if(strncmp(int_vars[0].name,w->word,len) == 0) {
			string_to_word(buff,&w);
			return;
		}
	}

	append_word(&global_env,init_word_str(buff));
}

update_umask(str)
char *str;
{
	int u;

	sscanf(str,"%o",&u);
	umask(u);
}

update_nice(str)
char *str;
{
	if(atoi(str) < 1) {
		report_error("mpsh-nice should be an integer greater than 0",str,0,0);
		set_env_str("mpsh-nice=10");
	}
}

delete_env(src)
char *src;
{
	int len;
	char *str;
	struct word_list *w, *last;

	len = strlen(src)+1;
	str = (char *) malloc(len);
	sprintf(str,"%s=",src);
	last = global_env;

	for(w=global_env->next; w; w=w->next) {
		if(strncmp(str,w->word,len) == 0) {
			last->next = w->next;
			free(str);
			return;
		}
		last = w;
	}
	report_error("variable not found",src,0,0);
	free(str);
}


update_null(str)
char *str;
{
	;
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
	int flags;
	int pipe_flags;

	append = strdup(str2);

	/* Save args & pipeline for appending later. */
	save_args = w->next;
	w->next = NULL;
	further_commands = curr->pipeline;
	curr->pipeline = NULL;
	flags = curr->flags;
	pipe_flags = curr->pipe_io_flags;

	curr = insert_parse_string(curr,str);
	if(!curr) return(0);

	last = find_last_word(&curr->words);
	append_string_to_word(last,append);
	free(append);

	while(curr->pipeline) curr = curr->pipeline;
	curr->pipeline = further_commands;
	curr->flags = flags;
	curr->pipe_io_flags = pipe_flags;
	while(curr->pipeline) curr = curr->pipeline;

	last = find_last_word(&curr->words);
	last->next = save_args;

	return(1);
}

