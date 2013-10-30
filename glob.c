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

glob.c:

	filename globbing

*/




#include <stdio.h>
#include <glob.h>

#ifdef LINUX
#include <stdlib.h>
#include <string.h>
#endif

#ifdef BSD
#include <stdlib.h>
#include <string.h>
#endif


#include "mpsh.h"


expand_globbing(initial_command)
struct command *initial_command;
{
	struct command *command;
	struct word_list *w, *previous, *next_orig_word;
	glob_t gl;
	int i;
	int len;
	int show_dot_files;


	command = initial_command;

	while(command) {
		w = command->words;
		check_command_glob(command->words);
		w = w->next;
		while(w) {
			if(
				index(w->word,'?') != 0 ||
				index(w->word,'*') != 0 ||
				index(w->word,'[') != 0
			) {
				if(glob(w->word,GLOB_NOCHECK,NULL,&gl) == 0) {
					/* Copy glob expansion from glob_t struct into wordlist */
					next_orig_word = w->next;
					i=0;
					if(w->word[0] == '.')
							show_dot_files = 1;
					else	show_dot_files = 0;
					for(i=0; gl.gl_pathv[i]; i++) {
						if(!show_dot_files && gl.gl_pathv[i][0] == '.')
							continue;
						len = strlen(gl.gl_pathv[i]) + 1;
						w->space = len;
						w->word = (char *) malloc(len);
						strcpy(w->word,gl.gl_pathv[i]);
						w->next = (struct word_list *)
							malloc(sizeof(struct word_list));
						previous = w;
						w = w->next;
					}
					w = previous;
					previous->next = next_orig_word;


				}
				globfree(&gl);
			}

			w = w->next;
		}
		command = command->pipeline;
	}
}


