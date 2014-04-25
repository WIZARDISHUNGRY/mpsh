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

scripts.c:

	run mpsh scripts

*/




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#ifdef LINUX
#endif

#ifdef BSD
#endif

#include "mpsh.h"




run_script(name,quiet)
char *name;
int quiet;
{
	FILE *fp;
	char *buff;
	int len;


	fp = fopen(name,"r");
	if(!fp) {
		if(!quiet) report_error("Error running script",name,0,1);
		return(1);
	}

	buff = (char *) malloc(BUFF_SIZE);

	while(fgets(buff,BUFF_SIZE,fp) != NULL) {
		len = strlen(buff);
		if(len > 0) buff[len-1] = '\0';
		if(buff[0] && buff[0] != '#') {
			parse_depth = 0;
			parse_and_run(buff,NONINTERACTIVE);
		}
	}

	fclose(fp);
	free(buff);
	return(0);
}

run_script_with_args(argc,argv,quiet)
int argc;
char *argv[];
int quiet;
{
	char buff[256];
	char *total_args;
	int len;
	int ret;
	int i;

	len = 2;
	for(i=2; i < argc; i++) len += strlen(argv[i]) + 1;
	len++;

	total_args = malloc(len);
	strcpy(total_args,"*=");

	for(i=2; i < argc; i++) {
		strcat(total_args,argv[i]);
		if(i+1 < argc)
			strcat(total_args," ");
		sprintf(buff,"%d=%s",i-1,argv[i]);
		set_env(buff);
	}

	set_env(total_args);
	free(total_args);
	sprintf(buff,"#=%d",argc-2);
	set_env(buff);

	ret = run_script(argv[1],quiet);

	return(ret);
}

