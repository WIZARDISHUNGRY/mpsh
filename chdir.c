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

chdir.c:

	functions related to changing the current directory

*/




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef LINUX
#endif

#ifdef BSD
#endif




#include "mpsh.h"

#define CDHISTORY_SIZE 64
char *cdhistory[CDHISTORY_SIZE];

int show_cdhistory_sub;

int cdhistory_next;

init_cdhistory() {
	int i;
	char *pt;

	for(i=0; i<CDHISTORY_SIZE; i++)
		cdhistory[i] = NULL;

	cdhistory_next = 0;

	/* Add history entry */
	pt = dup_cwd();
	if(pt) {
		cdhistory[cdhistory_next] = pt;
		cdhistory_next = (cdhistory_next+1)%CDHISTORY_SIZE;
		if(cdhistory[cdhistory_next])
			free(cdhistory[cdhistory_next]);
		cdhistory[cdhistory_next] = NULL;
	}
}


show_cdhistory() {
	int i;
	int h;

	for(i=0; i<CDHISTORY_SIZE; i++) {
		h = cdhistory_next+i-CDHISTORY_SIZE;
		if(h >= 0) {
			if(cdhistory[h])
				printf("%2d %s\n",h,cdhistory[h]);
		}
	}

}

builtin_cd(command,which)
struct command *command;
int which;
{
	int ret;
	char *arg;
	char *pt;
	int len, h, i, j;
	char *cdh;
	int sl;


	if(command->words->next == NULL) 
		arg = NULL;
	else
		arg = command->words->next->word;

	/*

	PARENT:

		cd
		cd dir
		cd !whatever
		cd -c

	CHILD:

		cd -s

	*/

	if(which == CHILD) {
		if(arg && strcmp(arg,"-s") == 0) {
			show_cdhistory();
			return(1);
		} else {
			return(0);
		}
	} else {
		if(arg && strcmp(arg,"-s") == 0) {
			return(0);
		}
	}

	if(arg && strcmp(arg,"-c") == 0) {
		init_cdhistory();
		return(1);
	}
		

	if(arg == NULL) 
		arg = get_env("HOME");

	if(arg[0] == '!') { /* CD history */
		pt = arg+1;
		len = strlen(pt);

		if(isdigit(pt[0])) {
			h = atoi(pt);
			arg = cdhistory[h%CDHISTORY_SIZE];
			if(show_cdhistory_sub) puts(arg);
			goto found;
		} else for(i=CDHISTORY_SIZE-2; i>=0; i--) {
			h = cdhistory_next+i;
			cdh = cdhistory[h%CDHISTORY_SIZE];
			if(cdh) {
				sl = strlen(cdh);
				for(j=0; j<sl; j++) {
					if(strncmp(pt,cdh+j,len) == 0) {
						arg = cdh;
						if(show_cdhistory_sub) puts(arg);
						goto found;
					}
				}
			}
		}
		report_error("Directory not found",arg,0,0);
		return(2);
	}

	found:

	ret = change_dir(arg);
	return(ret);
}

change_dir(arg)
char *arg;
{
	char *pt;
	int ret;

	ret = chdir(arg);
	if(ret == -1) {
		report_error("Error changing directory",arg,0,1);
		return(2);
	}

	pt = dup_cwd();
	if(pt) {
		cdhistory[cdhistory_next] = pt;
		cdhistory_next = (cdhistory_next+1)%CDHISTORY_SIZE;
		if(cdhistory[cdhistory_next])
			free(cdhistory[cdhistory_next]);
		cdhistory[cdhistory_next] = NULL;
	}
	return(1);
}


update_cdhistory_setting(str)
char *str;
{
	show_cdhistory_sub = atoi(str);
}


