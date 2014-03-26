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

prompt.c:



*/



#include <stdio.h>

#ifdef LINUX
#include <stdlib.h>
#include <string.h>
#endif

#ifdef BSD
#include <stdlib.h>
#include <string.h>
#endif

#include "mpsh.h"

char *prompt_string;
#define DEFAULT_PROMPT "mpsh$ "

init_prompt() {
	prompt_string = NULL;
	update_prompt_string(get_env("mpsh-prompt"));
	if(!prompt_string) prompt_string = DEFAULT_PROMPT;
}

display_prompt() {
	fputs(prompt_string,stdout);
	fflush(stdout);
}

char *get_prompt() {
	char *tmp;

	if(prompt_string[0] == '!') {
		tmp = read_from_command(prompt_string+1);
		return(tmp);
	}

	return(prompt_string);
}

update_prompt_string(str)
char *str;
{
	char *pt;

	if(str) {
		pt = (char *) malloc(strlen(str)+1);
		strcpy(pt,str);
		prompt_string = pt;
	}
}


