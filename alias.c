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

alias.c:

	alias functions

*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#ifdef LINUX
#endif

#ifdef BSD
#endif


#include "mpsh.h"


struct alias {
	char *name;
	char *text;
	struct alias *next;
} ;

struct alias *aliases;
struct alias *new_alias();


init_aliases() {
	aliases = NULL;
}

builtin_alias(command,which)
struct command *command;
int which;
{
	char *arg;

	if(!command->words->next) {
		report_error("Missing alias option",NULL,0,0);
		return(1);
	}

	arg = command->words->next->word;

	if(arg[0] == '-') {
		if(strcmp(arg,"-s") == 0) {
			show_aliases();
			return(1);
		}
		if(strcmp(arg,"-q") == 0) {
			show_aliases_quotes();
			return(1);
		}

		if(strcmp(arg,"-d") == 0) {
			if(command->words->next->next)
				delete_alias(command->words->next->next->word);
			else
				report_error("alias -d needs an argument",NULL,0,0);
			return(1);
		}

		report_error("unknown argument",arg,0,0);
		return(1);
	}

	if(index(arg,'=')) {
		set_alias(arg);
		return(1);
	}

	report_error("syntax error",arg,0,0);
	return(1);
}

show_aliases_quotes() {
	struct alias *a;

	for(a=aliases; a; a=a->next) {
		printf("%s=\"%s\"\n",a->name,a->text);
	}
}


show_aliases() {
	struct alias *a;
	int len, widest;

	widest = 0;
	for(a=aliases; a; a=a->next) {
		len = strlen(a->name);
		if(len > widest) widest = len;
	}

	for(a=aliases; a; a=a->next) {
		printf("%-*s = %s\n",widest,a->name,a->text);
	}
}

delete_alias(arg)
char *arg;
{
	struct alias *a, *prev;

	prev = NULL;

	for(a=aliases; a; a=a->next) {
		if(strcmp(arg,a->name) == 0) {
			if(prev)
				prev->next = a->next;
			else
				aliases = a->next;
			free(a->text);
			free(a->name);
			free(a);
			return;
		}
		prev = a;
	}

	report_error("alias not found",arg,0,0);
}

set_alias(arg)
char *arg;
{
	char *pt;
	struct alias *a;

	pt = index(arg,'=');
	*pt++ = '\0';

	/* Check if alias already exists */
	for(a=aliases; a; a=a->next) {
		if(strcmp(arg,a->name) == 0) {
			free(a->text);
			a->text = strdup(pt);
			return;
		}
	}

	/* Nope. Create a new alias entry */
	if(!aliases) {
		aliases = new_alias(arg,pt);
	} else {
		for(a=aliases; a->next; a=a->next) ;
		a->next = new_alias(arg,pt);
	}
}

struct alias *new_alias(name,text)
char *name, *text;
{
	struct alias *a;

	a = malloc(sizeof(struct alias));
	a->name = strdup(name);
	a->text = strdup(text);
	a->next = NULL;
	return(a);
}

char *find_alias(arg)
char *arg;
{
	struct alias *a;
	int len;

	for(a=aliases; a; a=a->next)
		if(strcmp(arg,a->name) == 0)
			return(a->text);

	return(NULL);
}


