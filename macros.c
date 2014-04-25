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

marco.c:

	fns related to macros


*/


#include <stdio.h>
#include <stdlib.h>

#ifdef LINUX
#include <string.h>
#endif

#ifdef BSD
#include <string.h>
#endif


#include "mpsh.h"


struct macro {
	char *name;
	char *text;
	int len;
	struct macro *next;
} ;

struct macro *macros;
struct macro *new_macro();


char *expand_macros(src)
char *src;
{
	char *buff;
	char *dst;
	char *pt;
	struct macro *m;
	int dst_len;
	int increase;

	if(!macros) return(src);

	dst_len = strlen(src) + 32;
	dst = buff = malloc(dst_len);

	pt = src;

	while(*pt) {
		for(m=macros; m; m=m->next) {
			if(strncmp(pt,m->name,m->len) == 0) {
				/* substitute */
				increase = strlen(m->text) - m->len;
				if(increase > 0) {
					*dst = '\0';
					dst_len += increase;
					buff = realloc(buff,dst_len);
					for(dst=buff; *dst; dst++) ;
				}

				strcpy(dst,m->text);
				dst += strlen(m->text);

				pt += m->len;

				goto skip;
			}
		}
		*dst++ = *pt++;

		skip: ;
	}

	*dst = '\0';

	return(buff);
}




init_macros() {
	macros = NULL;
}

builtin_macro(command,which)
struct command *command;
int which;
{
	char *arg;
	struct macro *m, *next;

	if(command->words->next) 
		arg = command->words->next->word;
	else
		arg = NULL;

	if(which == PARENT) {
		if(arg == NULL) return(0);
		if(strcmp(arg,"-d") == 0) {
			if(command->words->next->next)
				delete_macro(command->words->next->next->word);
			else
				report_error("Missing macro -d argument",NULL,0,0);
			return(1);
		}
		if(strcmp(arg,"-c") == 0) {
			m=macros; 
			while(m) {
				next = m->next;
				free(m->text);
				free(m->name);
				free(m);
				m = next;
			}
			macros = NULL;
			return(1);
		}


		if(arg[0] != '-') {
			if(index(arg,'=')) {
				set_macro(arg);
				return(1);
			} else {
				report_error("Syntax error",arg,0,0);
				return(1);
			}
		}
		return(0);
	} else {

		if(arg == NULL) {
			show_macros();
			return(1);
		}

		if(arg[0] == '-') {
			if(strcmp(arg,"-s") == 0) {
				show_macros();
				return(1);
			}
			if(strcmp(arg,"-q") == 0) {
				show_macros_quotes();
				return(1);
			}
			if(strcmp(arg,"-h") == 0) {
				puts("usage: macro            # show macros");
				puts("       macro -s         # show macros");
				puts("       macro -q         # show macros, quoted");
				puts("       macro -d n       # delete [n]");
				puts("       macro -c         # clear all macros");
				puts("       macro name=value # set command macro [name] to [value]");
				return(1);
			}

			report_error("Unknown macro option",arg,0,0);
			return(1);
		}
	}

	report_error("Syntax error",arg,0,0);
	return(1);
}

show_macros_quotes() {
	struct macro *m;

	for(m=macros; m; m=m->next) {
		printf("%s=\"%s\"\n",m->name,m->text);
	}
}


show_macros() {
	struct macro *m;
	int widest;
	int n;
	int n_width;

	widest = 0;
	n = 0;

	for(m=macros; m; m=m->next) {
		if(m->len > widest) widest = m->len;
		n++;
	}

	n_width = 0;
	--n;
	while(n > 0) {
		n_width++;
		n /= 10;
	}

	n = 0;

	for(m=macros; m; m=m->next) {
		printf("%*d %-*s = %s\n",n_width,n,widest,m->name,m->text);
		n++;
	}
}

delete_macro(arg)
char *arg;
{
	struct macro *m, *prev;
	int n, d;

	if(!isdigit(arg[0])) {
		report_error("Macro not found",arg,0,0);
		return;
	}
	d = atoi(arg);

	prev = NULL;

	n = 0;

	for(m=macros; m; m=m->next) {
		if(n == d) {
		/*
		if(strcmp(arg,m->name) == 0) {
		*/
			if(prev)
				prev->next = m->next;
			else
				macros = m->next;
			free(m->text);
			free(m->name);
			free(m);
			return;
		}
		n++;
		prev = m;
	}

	report_error("Macro not found",arg,0,0);
}

set_macro(arg)
char *arg;
{
	char *pt;
	struct macro *m;

	pt = index(arg,'=');
	*pt++ = '\0';

	/* Check if macro already exists */
	for(m=macros; m; m=m->next) {
		if(strcmp(arg,m->name) == 0) {
			free(m->text);
			m->text = strdup(pt);
			return;
		}
	}

	/* Nope. Create a new macro entry */
	if(!macros) {
		macros = new_macro(arg,pt);
	} else {
		for(m=macros; m->next; m=m->next) ;
		m->next = new_macro(arg,pt);
	}
}

struct macro *new_macro(name,text)
char *name, *text;
{
	struct macro *m;

	m = (struct macro *) malloc(sizeof(struct macro));
	m->name = strdup(name);
	m->text = strdup(text);
	m->len = strlen(name);
	m->next = NULL;
	return(m);
}

char *find_macro(arg)
char *arg;
{
	struct macro *m;

	for(m=macros; m; m=m->next)
		if(strcmp(arg,m->name) == 0)
			return(m->text);

	return(NULL);
}


