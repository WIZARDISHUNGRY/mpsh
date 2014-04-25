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

path.c:

	functions related to the command search path

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

struct word_list *search_path;

char *glob_path();

init_search_path() {
	search_path = NULL;
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

	if(pt == NULL || pt == (char *) -1) {
		report_error("PATH variable not found",NULL,0,0);
		return;
	}

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
					if(strcmp(dp->d_name,".") != 0 && 
						strcmp(dp->d_name,"..") != 0) {
						if(!command_path_list) 
							command_path_list = c =
								new_path_list(pth->word,dp->d_name);
						else {
							c->next = new_path_list(pth->word,dp->d_name);
							c = c->next;
						}
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
	struct path_list *c;
	struct stat st;
	char buff[256];
	char *ret;

	if(!arg[0] || arg[0] == '[') {
		return(NULL);
	}

	ret = glob_path(arg);
	if(ret) return(ret);

	if(index(arg,'/') != NULL) {
		return(strdup(arg));
	}

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

	/* No match found. Update search path and try again. */
	/* Then give up. */

	update_search_path();

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

	return(NULL);
}



char *glob_path(arg)
char *arg;
{
	struct path_list *c;
	char *pt;
	int len;

	if(index(arg,'?') != 0 || index(arg,'*') != 0 || index(arg,'[') != 0) {
		for(c=command_path_list; c; c=c->next) {
			if(c->command && fnmatch(arg,c->command,FNM_PATHNAME) == 0) {
				len = strlen(c->dir) + strlen(c->command) + 2;
				pt = (char *) malloc(len);
				sprintf(pt,"%s/%s",c->dir,c->command);
				return(pt);
			}
		}
	}
	return(NULL);
}

builtin_show_path(command,which)
struct command *command;
int which;
{
	char *arg;
	struct path_list *c;
	int widest;
	int len;
	char *pt;

	if(which == PARENT) return(0);

	if(command->words->next)
		arg = command->words->next->word;
	else
		arg = NULL;

	if(arg && strcmp(arg,"-h") == 0) {
		puts("usage: show-path         # Show command path cache");
		puts("       show-path command # Show path to command");
		return(1);
	}

	if(arg) {
		pt = find_path(arg);
		if(pt) puts(pt);
		else report_error("Command not found",arg,0,0);
		return(1);
	}

	widest = 0;

	for(c=command_path_list; c; c=c->next) {
		len = strlen(c->dir);
		if(len > widest) widest = len;
	}

	for(c=command_path_list; c; c=c->next)
		printf("%-*s %s\n",widest,c->dir,c->command);

	return(1);
}

