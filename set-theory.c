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

set-theory.c:

	set theory operations 

*/




#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>


#ifdef LINUX
#endif

#ifdef BSD
#endif

#include "mpsh.h"


/*

	FLAG_SETOP
	FLAG_SETOP_UNION
	FLAG_SETOP_INTER
	FLAG_SETOP_DIFF
	FLAG_SETOP_SYMM
	FLAG_SETOP_SUBSET
	FLAG_SETOP_SUBSET_REV

	|U - union of A U B.
	|^ - intersection of A^B
	|\ - difference A\B (elements of A not in B)
	|- - difference?
	|o - symmetric difference (elements in only one of A or B)
	|< - conditional, A is a subset of B?
	|> - conditional, B is a subset of A?
	|= - conditional, A equals B?

*/

setop_union(src)
struct command *src;
{
	struct word_list *w1, *w2;
	struct word_list *w;

	read_two_sets(src,&w1,&w2);

	for(w=w1; w->next; w=w->next) ;
	w->next = w2;

	/*
	cleanup_set(w1);
	*/
	display_set(w1);
}


setop_intersection(src)
struct command *src;
{
	struct word_list *w1, *w2;
	struct word_list *w;
	struct word_list *s1, *s2;
	struct word_list *dest, *d;

	read_two_sets(src,&w1,&w2);

	dest = NULL;

	for(s1=w1; s1; s1=s1->next) {
		for(s2=w2; s2; s2=s2->next) {
			if(strcmp(s1->word,s2->word) == 0) {
				if(!dest)
					dest = d = init_word_str(s1->word);
				else {
					d->next = init_word_str(s1->word);
					d = d->next;
				}
			}
		}
	}

	/*
	cleanup_set(dest);
	*/
	display_set(dest);
}


setop_subset(src)
struct command *src;
{
	struct word_list *w1, *w2;
	struct word_list *s1, *s2;

	read_two_sets(src,&w1,&w2);
	cleanup_set(w1);
	cleanup_set(w2);

	s1 = w1;
	s2 = w2;

	/* SUBSET */

	for(s1=w1; s1; s1=s1->next) {
		for(s2=w2; s2; s2=s2->next) {
			if(strcmp(s1->word,s2->word) == 0) 
				goto skip;
		}
		return(0);
		skip: ;
	}
	return(1);
}



setop_subset_rev(src)
struct command *src;
{
	struct word_list *w1, *w2;
	struct word_list *s1, *s2;

	read_two_sets(src,&w2,&w1);
	cleanup_set(w1);
	cleanup_set(w2);

	s1 = w1;
	s2 = w2;

	/* SUBSET */

	for(s1=w1; s1; s1=s1->next) {
		for(s2=w2; s2; s2=s2->next) {
			if(strcmp(s1->word,s2->word) == 0) 
				goto skip;
		}
		return(0);
		skip: ;
	}
	return(1);
}

setop_equals(src)
struct command *src;
{
	struct word_list *w1, *w2;
	struct word_list *s1, *s2;

	read_two_sets(src,&w1,&w2);
	cleanup_set(w1);
	cleanup_set(w2);

	sort_set(&w1);
	sort_set(&w2);

	s1 = w1;
	s2 = w2;

	for(;;) {
		if(strcmp(s1->word,s2->word) != 0)
			return(0);

		s1 = s1->next;
		s2 = s2->next;

		if(!s1 && s2) return(0);
		if(s1 && !s2) return(0);
		if(!s1 && !s2) return(1);
	}
}


setop_difference(src)
struct command *src;
{
	struct word_list *w1, *w2;
	struct word_list *w;
	struct word_list *s1, *s2;
	struct word_list *dest, *d;

	read_two_sets(src,&w1,&w2);

	dest = NULL;

	for(s1=w1; s1; s1=s1->next) {
		for(s2=w2; s2; s2=s2->next) {
			if(strcmp(s1->word,s2->word) == 0) 
				goto skip;
		}
		if(!dest)
			dest = d = init_word_str(s1->word);
		else {
			d->next = init_word_str(s1->word);
			d = d->next;
		}
		skip: ;
	}

	/*
	cleanup_set(dest);
	*/
	display_set(dest);
}

setop_symmetric(src)
struct command *src;
{
	struct word_list *w1, *w2;
	struct word_list *w;
	struct word_list *s1, *s2;
	struct word_list *dest, *d;

	read_two_sets(src,&w1,&w2);

	dest = NULL;

	for(s1=w1; s1; s1=s1->next) {
		for(s2=w2; s2; s2=s2->next) {
			if(strcmp(s1->word,s2->word) == 0) 
				goto skip1;
		}
		if(!dest)
			dest = d = init_word_str(s1->word);
		else {
			d->next = init_word_str(s1->word);
			d = d->next;
		}
		skip1: ;
	}

	for(s1=w2; s1; s1=s1->next) {
		for(s2=w1; s2; s2=s2->next) {
			if(strcmp(s1->word,s2->word) == 0) 
				goto skip2;
		}
		if(!dest)
			dest = d = init_word_str(s1->word);
		else {
			d->next = init_word_str(s1->word);
			d = d->next;
		}
		skip2: ;
	}

	/*
	cleanup_set(dest);
	*/
	display_set(dest);
}

read_two_sets(src,dest1,dest2)
struct command *src;
struct word_list **dest1, **dest2;
{
	struct command *c;
	struct word_list *w;
	int p[2];
	FILE *in;
	char buff[1024];

	/* Removes setop args, pipeline settings, etc */
	c = init_command(src);
	c->words = src->words;
	c->flags |= src->flags & FLAG_GROUP;

	*dest1 = w = init_word();

	while(fgets(buff,1023,stdin)) {
		buff[strlen(buff)-1] = '\0';
		w->next = init_word_str(buff);
		w = w->next;
	}

	pipe(p);
	in = fdopen(p[0],"r");

	*dest2 = w = init_word();

	call_exec(c,-1,p[1]);
	while(fgets(buff,1023,in)) {
		buff[strlen(buff)-1] = '\0';
		w->next = init_word_str(buff);
		w = w->next;
	}
	fclose(in);
	close(p[1]);

}

int sort_string(s1,s2)
const void *s1, *s2;
{  
	return strcmp(*(char * const *)s1, *(char * const *)s2);
}  


sort_set(srcpt)
struct word_list **srcpt;
{
	struct word_list *src;
	struct word_list *w;
	char **buff;
	int num, i;

	src = *srcpt;

	num = 0;
	for(w = src; w->next; w=w->next) num++;

	buff = (char **) malloc(sizeof(char *)*(num+1));

	i = 0;
	for(w = src; w; w=w->next) {
		buff[i] = w->word;
		i++;
	}
	buff[i] = NULL;

	qsort(buff,num+1,sizeof(char *),sort_string);

	src = w = init_word_str(buff[0]);
	for(i=1; i<=num; i++) {
		w->next = init_word_str(buff[i]);
		w = w->next;
	}

	free(buff);

	*srcpt = src;
}

cleanup_set(src) /* Remove duplicates */
struct word_list *src;
{
	struct word_list *s0, *s1, *s2;
	struct word_list *tmp, *old;


	for(s1=src; s1; s1=s1->next) {
		old = s1;
		s2 = old->next;
		while(s2) {
			if(strcmp(s1->word,s2->word) == 0) {
				old->next = s2->next;
				s2 = s2->next;
			} else {
				old = s2;
				s2 = s2->next;
			}
			

			/*
				tmp = s2;
				old->next = s2->next;
				s2 = old->next;
			} else {
				old = old->next;
				s2 = old->next;
			}
			*/

		}
	}
}


display_set(set) /* Display a set. */
struct word_list *set;
{
	struct word_list *w;

	cleanup_set(set);

	for(w=set; w; w=w->next) {
		if(w->word[0]) puts(w->word);
	}
}


show_set(set) /* Display a set. */
struct word_list *set;
{
	struct word_list *w;
	struct word_list *dest;
	int depth;

	dest = NULL;
	depth = 0;

	for(w=set; w; w=w->next) {
		if(strcmp(w->word,"{") == 0) depth++;
		if(strcmp(w->word,"}") == 0) depth--;

		if(depth < 0) {
			return(1);
		}

		if(depth > 0) 
			printf("%s ",w->word);
		else
			if(w->word[0]) {
				puts(w->word);
			}
	}

	report_error("Mismatched set",NULL,0,0);
	return(0);
}


/* The settings for set operators should be in the command *after* the
	one they're initially parsed in. */

shift_set_operators(comm)
struct command *comm;
{
	int new, old;
	struct command *c;

	new = old = comm->flags;
	for(c=comm; c; c=c->pipeline) {
		if(c->pipeline) {
			new = c->pipeline->flags;
			c->pipeline->flags = 
				(c->pipeline->flags & FLAG_LOWER) | (old & FLAG_SETOP);
			old = new;
		}
	}
	comm->flags = (comm->flags & FLAG_LOWER);

}

