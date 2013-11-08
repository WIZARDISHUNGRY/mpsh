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

error.c:

	error reporting functions


*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

#ifdef LINUX
#endif

#ifdef BSD
#endif

#include "mpsh.h"

char *quote_type[] = {
	"none",
	"double quote",
	"single quote",
	"parentheses",
	"curly bracket",
	"square bracket",
	NULL
} ;



int error_level;


init_error_level() {
	error_level = 1;
}

update_error_level(str)
char *str;
{
	error_level = atoi(str);
}

report_error(msg,str,ch,err)
char *msg;
char *str;
int ch;
int err;
{
	if(error_level > 0) {
		fprintf(stderr,"mpsh: %s",msg);
	} else return;
	if(error_level > 1) {
		if(str) fprintf(stderr," [%s]",str);
		if(ch) fprintf(stderr," [%c]",ch);
	}
	if(error_level > 2) {
		if(err) fprintf(stderr," %s",strerror(errno));
	}
	fprintf(stderr,"\n");
}

report_mismatched_something(command)
struct command *command;
{
	struct word_list *w;
	char buff[64];
	int depth;
	int q;

	w = find_last_word(&command->words);

	depth = command->quote_depth;
	q = 0;

	if(depth < 1) {
		switch(command->ch) {
			case '\"':
				q = QUOTE_DOUBLE;
				break;
			case '\'':
				q = QUOTE_SINGLE;
				break;
			case '(':
			case ')':
				q = QUOTE_PAREN;
				break;
			case '{':
			case '}':
				q = QUOTE_BRACE;
				break;
			case '[':
			case ']':
				q = QUOTE_SQUARE;
				break;
		}
	} else {
		q = command->quote_stack[depth];
	}

	sprintf(buff,"Mismatched %s",quote_type[q]);
	if(command->ch != '\n')
		add_letter_to_word(w,command->ch);
	report_error(buff,w->word,0,0);
}


