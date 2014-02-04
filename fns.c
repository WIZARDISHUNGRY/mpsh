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

fns.c:

	generic functions for mpsh use

*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#ifdef LINUX
#endif

#ifdef BSD
#endif


#include "mpsh.h"



add_word(command,add)
struct command *command;
struct word_list *add;
{
	append_word(&command->words,add);
}

append_word(w_ptr,add)
struct word_list **w_ptr;
struct word_list *add;
{
	struct word_list *w;
	int len;
	if(*w_ptr == NULL) {
		w = *w_ptr = init_word();
	} else {
		w = *w_ptr;
		while(w->next) w = w->next;
		w->next = init_word();
		w = w->next;
	}
	if(add->space > w->space) {
		len = strlen(add->word)+1;
		w->space = len;
		free(w->word);
		w->word = (char *) malloc(len);
	}
	strcpy(w->word,add->word);
}

struct word_list *find_next_to_last_word(list_ptr)
struct word_list **list_ptr;
{
	struct word_list *w;
	if(*list_ptr == NULL)
		*list_ptr = init_word();
	w = *list_ptr;
	while(w->next && w->next->next) w = w->next;
	return(w);
}

struct word_list *find_last_word(list_ptr)
struct word_list **list_ptr;
{
	struct word_list *w;
	if(*list_ptr == NULL)
		*list_ptr = init_word();
	w = *list_ptr;
	while(w->next) w = w->next;
	return(w);
}

struct word_list *init_word() {
	struct word_list *w;
	w = (struct word_list *) malloc(sizeof(struct word_list));
	w->space = WORD_ALLOC;
	w->word = (char *) malloc(WORD_ALLOC);
	w->word[0] = '\0';
	w->next = NULL;
	return(w);
}

struct word_list *init_word_str(str)
char *str;
{
	int len;
	struct word_list *w;

	len = strlen(str)+1;
	w = (struct word_list *) malloc(sizeof(struct word_list));
	if(!w) {
		perror("Error allocating word_list struct");
		exit(1);
	}
	w->space = len;
	w->word = (char *) malloc(len);
	strcpy(w->word,str);
	w->next = NULL;
	return(w);
}

struct word_list *new_arg_word(command)
struct command *command;
{
	return(new_word(&command->words));
}

struct word_list *new_word(w)
struct word_list **w;
{
	while(*w)
		w = &((*w)->next);
	*w = init_word();
	return(*w);
}

struct word_list *string_to_word(str,w)
char *str;
struct word_list **w;
{
	int len;

	if(!(*w)) *w = init_word();

	len = strlen(str)+1;
	if(len > (*w)->space) {
		free((*w)->word);
		(*w)->word = (char *) malloc(len);
		(*w)->space = len;
	}
	strcpy((*w)->word,str);
	return(*w);
}

add_letter_to_word(w,ch)
struct word_list *w;
int ch;
{
	char *tmp;
	int len;

	len = strlen(w->word);
	if((len+1) >= w->space) {
		w->space += WORD_ALLOC;
		tmp = (char *) malloc(w->space);
		strcpy(tmp,w->word);
		free(w->word);
		w->word = tmp;
	}

	w->word[len] = ch;
	w->word[len+1] = 0;
}

append_string_to_word(w,str)
struct word_list *w;
char *str;
{
	char *tmp;
	int len;

	if(!str[0]) return;

	len = strlen(w->word) + strlen(str);
	if((len+1) >= w->space) {
		while((len+1) >= w->space)
			w->space += WORD_ALLOC;
		tmp = (char *) malloc(w->space);
		strcpy(tmp,w->word);
		free(w->word);
		w->word = tmp;
	}

	strcat(w->word,str);
}

struct command *init_command() {
	struct command *new;
	new = (struct command *) malloc(sizeof(struct command));
	new->text = NULL;
	new->expansion = NULL;
	new->dir = NULL;
	new->echo_text = NULL;
	new->words = NULL;
	new->flags = 0x0000;
	new->nice = 0;
	new->pipe_io_flags = 0x0000;
	new->file_io_flags = 0x0000;
	new->smp_num = 0;
	new->smp_id = 0;
	new->display_text = 0;
	new->job_handler = 0;
	new->pid = 0;
	new->stdout_filename = NULL;
	new->stderr_filename = NULL;
	new->handler_args = NULL;
	new->pipeline = NULL;
	new->state = 0;
	new->backslash = 0;
	new->quote_depth = 0;
	new->quote_stack[0] = 0;
	new->start_of_sub = 0;

	return(new);
}


free_command(command)
struct command *command;
{
	struct command *next;

	/* We don't free() c->text or c->dir because those are handed
		off to history[] as is.
	*/

	for(;;) {
		if(command->echo_text) free(command->echo_text);
		free_word_list(command->words);
		free_word_list(command->handler_args);
		free_word_list(command->stderr_filename);
		free_word_list(command->stdout_filename);
		next = command->pipeline;
		free(command);
		if(next) 
			command = next;
		else
			return;
	}
}

free_word_list(w)
struct word_list *w;
{
	while(w) {
		free(w->word);
		free(w);
		w = w->next;
	}
}

copy_word_list(src,dest_ptr)
struct word_list *src;
struct word_list **dest_ptr;
{
	while(src) {
		append_word(dest_ptr,src);
		src = src->next;
	}
}


char *words_to_string(src)
struct word_list *src;
{
	struct word_list *w;
	char *dest;
	int len;

	len = 0;
	for(w = src; w; w=w->next) {
		len += strlen(w->word)+1;
	}
	len += 2;

	dest = (char *) malloc(len);
	dest[0] = '\0';

	for(w = src; w; w=w->next) {
		if(dest[0]) strcat(dest," ");
		strcat(dest,w->word);
	}
	return(dest);
}

char *end_of_string(str)
char *str;
{
	char *pt;

	while(*pt++);

	return(pt);
}

char *dup_cwd() {
	char *tmp, *pt;
	tmp = getcwd(NULL,1024);
	if(tmp == NULL) return(strdup("-"));
	pt = strdup(tmp);
	free(tmp);
	return(pt);
}

char *get_command_expansion(command)
struct command *command;
{
	struct command *curr;
	struct word_list *w;
	char *dest;
	int len;

	len = 0;
	for(curr=command; curr; curr=curr->pipeline) {
		for(w = curr->words; w; w=w->next) {
			len += strlen(w->word)+1;
		}
		len += 2;
	}

	dest = (char *) malloc(len);
	dest[0] = '\0';

	len = 0;
	for(curr=command; curr; curr=curr->pipeline) {
		for(w = curr->words; w; w=w->next) {
			if(dest[0]) strcat(dest," ");
			strcat(dest,w->word);
		}
		if(curr->pipeline) {
			if(curr->flags & FLAG_PIPE) 
				strcat(dest,"|");
			else
				strcat(dest,";");
		}
	}

	return(dest);
}

display_word_list(w)
struct word_list *w;
{
	while(w) {
		if(w->word[0]) puts(w->word);
		w = w->next;
	}
}


