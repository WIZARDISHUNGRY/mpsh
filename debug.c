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

debug.c:

	debug functions for mpsh development use.

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

int errno;



display_command(command)
struct command *command;
{
	struct word_list *w;

	printf("PARSED AS:\n");

	for(;;) {

		for(w = command->words; w->next; w=w->next)
			printf("{%s} ",w->word);
		printf("{%s} ",w->word);
		if(command->flags & FLAG_REDIRECT) {
			printf("> ");
			if(command->stdout_filename != NULL) {
				printf("(stdout) ");
				printf("[%s] ",command->stdout_filename->word);
			}
			if(command->stderr_filename) {
				printf("(stderr) ");
				printf("[%s] ",command->stderr_filename->word);
			}
			if(command->file_io_flags & IO_OUT_APPEND ||
				command->file_io_flags & IO_ERR_APPEND)
				printf("(append) ");
		}
		if(command->flags & FLAG_PIPE) {
			printf("| ");
			if(command->pipe_io_flags & IO_OUT)
				printf("(stdout) ");
			if(command->pipe_io_flags & IO_ERR)
				printf("(stderr) ");
		}
		if(command->flags & FLAG_BACK) {
			printf("& ");
			if(command->flags & FLAG_BATCH)
				printf("(queue) ");
			if(command->flags & FLAG_SMP)
				printf("[n = %d] ",command->smp_num);
		}

		if(command->handler_args) {
			printf("Handler arguments: ");
			for(w=command->handler_args; w->next; w=w->next)
				printf("%s ",w->word);
			puts(w->word);
		}


		if(command->pipeline) {
			if(!(command->flags & FLAG_PIPE)) {
				if(command->flags & FLAG_COND)
					printf(" ; (if) ");
				else
					printf(" ; ");
			}

			puts("");
			command = command->pipeline;
		} else break;
		printf("FLAGS: %04x\n",command->flags);
	}
	puts("");
	printf("FLAGS: %04x\n",command->flags);
}


log_err(str)
char *str;
{
	FILE *fp;

	fp = fopen("err","a");
	fprintf(fp,"%s (%d)\n",str,errno);
	fclose(fp);
}

